/*
 * hwwbus-hb.cpp
 *
 * Created on: April 05, 2022
 * Author: Konstantin Klubnichkin
 * Company: Yandex LLC
 */

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <regex>
#include <map>
#include <algorithm>
#include <ctime>
#include <netinet/in.h>

#include "hwwbus-hb.hpp"
#include <CLI/CLI.hpp>
#include <unistd.h>
#include "boost/asio.hpp"
#include <boost/algorithm/string/classification.hpp> // Include boost::for is_any_of
#include <boost/algorithm/string/split.hpp> // Include for boost::split
#include <chrono>
#include <sys/sysinfo.h>
#include <algorithm>

using namespace boost::asio;

bool DEBUG = false;
std::string ipmi_iface = "eth0";
char  MACAddress[13];
uint16_t interval = 60;
std::string url = "hwwbus.haas.yandex.net";
uint16_t port = 9443;

struct HeartbeatPacket udpPacket;

std::string readIPv6()
{
    std::string if_inet6 = "/proc/net/if_inet6";
    std::stringstream ss_ipv6_addr;
    std::ifstream input_file(if_inet6);
    if (input_file.is_open())
    {
        std::string line;
        while (std::getline(input_file, line, '\n'))
        {
            std::vector<std::string> fields;
            boost::split(fields, line, boost::is_any_of(" "), boost::token_compress_on);

            /* Check only our interface and only global address */
            if (fields[5] != ipmi_iface)
                continue;
            if (fields[3] != "00")
                continue;

            /* Now form a valid IPv6 addrss from line of hex numbers */
            for(int i=0; i < 8; i++)
            {
                ss_ipv6_addr << fields[0][i*4 + 0];
                ss_ipv6_addr << fields[0][i*4 + 1];
                ss_ipv6_addr << fields[0][i*4 + 2];
                ss_ipv6_addr << fields[0][i*4 + 3];
                if (i != 7)
                    ss_ipv6_addr << ":";
            }
            if (DEBUG)
                std::cerr << "Global IPv6: " << ss_ipv6_addr.str() << "\n";
            break;
        }
        input_file.close();
        return ss_ipv6_addr.str();
    } else
    {
        if (DEBUG)
            std::cerr << "Unable to open file " << if_inet6 << "\n";
        return "";
    }
}

int readMacAddress()
{
    std::string path = "/sys/class/net/" + ipmi_iface + "/address";
    if (DEBUG)
        std::cerr << "Path:" << path << "\n";
    std::ifstream input_file(path);
    if (input_file.is_open())
    {
        std::string mac;
        input_file >> mac;
        if (DEBUG)
            std::cerr << "MAC: " << mac << "\n";
        input_file.close();
        mac.erase(std::remove(mac.begin(), mac.end(), ':'), mac.end());
        bzero(&MACAddress, sizeof(MACAddress));
        memcpy(&MACAddress, mac.c_str(), 12);
        return 0;
    } else
    {
        if (DEBUG)
            std::cerr << "Unable to open file " << path << "\n";
        return -1;
    }
}

static std::string getModel()
{
    static std::string model;
    if (! model.empty())
        return model;

    std::string path = "/usr/share/openrack/machine";
    std::ifstream input_file(path);
    if (input_file.is_open())
    {
        std::string line;
        while (std::getline(input_file, line))
        {
            if (DEBUG)
                std::cerr << "machine: " << line << "\n";
            if (line.find("model=") != std::string::npos)
            {
                model = line.substr(line.find("=") + 1);
                break;
            }
        }
        input_file.close();
        if (! model.empty())
            return model;
    }
    else
    {
        if (DEBUG)
            std::cerr << "Unable to open file " << path << "\n";
    }
    return "unknown";
}

static std::string getVersion()
{
    static std::string version;
    if (! version.empty())
        return version;

    std::string path = "/etc/os-release";
    std::ifstream input_file(path);
    if (input_file.is_open())
    {
        std::string line;
        while (std::getline(input_file, line))
        {
            if (DEBUG)
                std::cerr << "os-release: " << line << "\n";
            if (line.find("VERSION_ID=") != std::string::npos)
            {
                std::string s = line.substr(line.find("=") + 1);
                s.erase(std::remove(s.begin(), s.end(), '"'), s.end());
                version = s;
                break;
            }
        }
        input_file.close();
        if (! version.empty())
            return version;
    }
    else
    {
        if (DEBUG)
            std::cerr << "Unable to open file " << path << "\n";
    }
    return "unknown";
}

int PrepareUDPPacket()
{
    bzero(&udpPacket, sizeof(struct HeartbeatPacket));
    udpPacket.version = htons(0x0001);
    udpPacket.interval = htons(interval);
    memcpy(&(udpPacket.mac), &MACAddress, 12);

    std::time_t t = std::time(0);
    struct sysinfo x;
    long int uptime = 0;
    if (sysinfo(&x) == 0)
    {
        uptime = x.uptime;
    }

    std::string ipv6_addr = readIPv6();
    if (ipv6_addr == "")
        snprintf(udpPacket.json, 256,
                "{\"ipmi\": \"%s\", \"ts\": %ld, \"uptime\": %ld, \"model\": \"%s\", \"version\": \"%s\"}",
                MACAddress, t, uptime, getModel().c_str(), getVersion().c_str());
    else
        snprintf(udpPacket.json, 256,
                "{\"ipmi\": \"%s\", \"ipv6\": \"%s\", \"ts\": %ld, \"uptime\": %ld, \"model\": \"%s\", \"version\": \"%s\"}",
                MACAddress, ipv6_addr.c_str(), t, uptime, getModel().c_str(), getVersion().c_str());
    if (DEBUG)
        printf("JSON prepared:%s\n", udpPacket.json);
    return 0;
}

int SendUDP()
{
    try
    {
        io_service io_service;
        ip::udp::socket socket(io_service);
        ip::udp::endpoint remote_endpoint;
        ip::udp::resolver resolver(io_service);
        ip::udp::resolver::query query(url, std::to_string(port));
        ip::udp::resolver::iterator iter = resolver.resolve(query);
        remote_endpoint = *iter;
        socket.open(ip::udp::v6());
        boost::system::error_code err;
        socket.send_to(buffer(&udpPacket, sizeof(udpPacket)), remote_endpoint, 0, err);
        socket.close();
        return 0;
    } catch (const boost::system::system_error& ex)
    {
        if (DEBUG)
            std::cerr << "Error:" << ex.what() << " at " << ex.code() << "\n";
    }
    return 1;
}

int main(int argc, char* argv[])
{
    CLI::App app("HWWBUS Hearbeat service");
    app.add_option("-v,--verbose", DEBUG, "print debug output");
    app.add_option("--interface", ipmi_iface, "IPMI network interface, eth0 by default");
    app.add_option("--interval", interval, "cycle interval in seconds, 60s by default");
    app.add_option("--url", url, "cycle interval in seconds, 60s by default");
    app.add_option("--port", port, "cycle interval in seconds, 60s by default");
    CLI11_PARSE(app, argc, argv);

    if (readMacAddress() < 0)
    {
        std::cerr << "Unable to read MAC address, exiting\n";
        return 1;
    }

    while(1)
    {
        PrepareUDPPacket();
        SendUDP();
        sleep(interval);
    }

    return 0;
}
