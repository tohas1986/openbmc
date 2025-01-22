/*
 *  rmtparser.cpp
 *
 *  Created on: Apr 05, 2021
 *  Author: Nikita Vedeneev
 *  Company: Yandex LLC
 */

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <regex>
#include <sstream>
#include <map>
#include <iomanip>
#include <string>
#include "nlohmann/json.hpp"

#include <boost/container/flat_map.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio.hpp>

#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/interface.hpp>
#include <sdbusplus/message.hpp>
#include <CLI/CLI.hpp>
#include <sdbusplus/timer.hpp>

bool DEBUG = false;

#define PROPERTY_INTERFACE "org.freedesktop.DBus.Properties"

struct rankMargin {

    std::vector<std::string> dimmDames;
    std::vector<std::string> rankMardinValues = {"RxDqs-", "RxDqs+",  "RxV-",  "RxV+",  "TxDq-",  "TxDq+",  "TxV-",  "TxV+",  "Cmd-",  "Cmd+",  "CmdV-",  "CmdV+",  "Ctl-",  "Ctl+"};
    std::vector<std::string> laneMardinValues = {"RxDqs-", "RxDqs+",  "RxV-",  "RxV+",  "TxDq-",  "TxDq+",  "TxV-",  "TxV+"};

    std::map<std::string, std::vector<double>> rankMargin;
    std::map<std::string, std::vector<double>> laneMargin;
};

struct mbistEyeDataMargin
{
    std::vector<std::string> readDataEyeMarginValues = { "RxV+", "Dq+", "RxV-", "Dq-" };
    std::vector<std::string> writeDataEyeMarginValues = { "TxV+", "Dq+", "TxV-", "Dq-" };

    std::map<std::string, std::vector<int>> writeDataEyeMargin;
    std::map<std::string, std::vector<int>> readDataEyeMargin;
};

void printMap(std::map<std::string, std::vector<double>> myMap)
{
    for(auto it = myMap.cbegin(); it != myMap.cend(); ++it)
    {
        std::cout << it->first << ": ";
        for (auto i : it->second)
        {
            std::cout << i << "; ";
        }
        std::cout << "\n";
    }
}

bool isNumber(const std::string& s)
{
    try
    {
        std::stod(s);
    }
    catch(...)
    {
        if(DEBUG)
        {
            std::cerr << "Given value is not double: "<< s << "\n";
        }
        return false;
    }
    return true;
}

std::vector<double> split(std::string &s) {
    std::stringstream ss(s);
    std::string item;
    std::vector<std::string> temp;
    std::vector<double> elems;
    int tempSizeMax = 13;

    while (std::getline(ss, item, ' ')) {

        temp.push_back(item);

    }

    for (int i = 0; i < temp.size(); ) {
        if (temp[i].size() == 0) {
            temp.erase(temp.begin() + i);
        } else ++i;
    }

    for(int i=0; i<tempSizeMax; i++)
    {
        if(!isNumber(temp[i+1]))
        {
            continue;
        }
        elems.push_back(std::abs(std::stod(temp[i+1])));
    }

    return elems;
}

void dimmMatch(std::string& line, bool& matchReadyFlag, std::vector<double>& MarginResults, std::string& dimmName, const std::regex& regexp)
{
    std::smatch dN;

    std::regex_search(line, dN, regexp);

    MarginResults.clear();
    dimmName.clear();

    if(dN.empty())
    {
        matchReadyFlag = false;
    }
    else
    {
        matchReadyFlag = true;

        for (auto x : dN)
        {
            dimmName += x;
        }

        MarginResults = split(line);
    }
}

void findRankMarginResults(rankMargin& rankmargin, bool& matchReadyFlag, const std::regex& regexpRank)
{
    std::ifstream input("/var/log/rmt-output.log");
    std::string line;
    std::vector<double> rankMarginResults;
    std::string dimmName;

    bool startCounter = false;

    while (getline(input, line))
    {
        if(startCounter)
        {
            dimmMatch(line, matchReadyFlag, rankMarginResults, dimmName, regexpRank);

            if(rankMarginResults.size() > 0)
            {
                rankmargin.rankMargin[dimmName] = rankMarginResults;
            }
        }

        if(line.find("Rank Margin") != std::string::npos)
        {
            if(DEBUG) std::cout << "Found Rank Margin entry..." << "\n";
            startCounter = true;
        }

        if(line.find("Lane Margin") != std::string::npos)
        {
            if(DEBUG) std::cout << "No more Rank Margin entries..." << "\n";
            startCounter = false;
            break;
        }
    }

    input.close();
}

void findLaneMarginResults(rankMargin& rankmargin, bool& matchReadyFlag, const std::regex regexpLane)
{
    std::ifstream input("/var/log/rmt-output.log");
    std::string line;
    std::vector<double> laneMarginResults;
    std::string dimmName;

    bool startCounter = false;

    while (getline(input, line)) {
        if (startCounter)
        {
            dimmMatch(line, matchReadyFlag, laneMarginResults, dimmName, regexpLane);

            if (laneMarginResults.size() > 0)
            {
                rankmargin.laneMargin[dimmName] = laneMarginResults;
            }
            laneMarginResults.clear();
        }
        if (line.find("Lane Margin") != std::string::npos)
        {
            std::cout << "Found Lane Margin entries..." << "\n";
            startCounter = true;
        }
        if (line.find("STOP_BSSA_RMT") != std::string::npos)
        {
            std::cout << "No more Lane Margin entries..." << "\n";
            startCounter = false;
            break;
        }
    }

    input.close();
}

void makeJson(rankMargin& rankmargin)
{
    std::ofstream output("/var/log/rmt.json");

    nlohmann::ordered_json rmtJson;
    std::vector<double> tempValueStore;

    rmtJson["Help"] = { "CPU: 0-1", "Channel 0-5", "DIMM: 0,1", "Rank: 0-3"};

    for(auto it = rankmargin.rankMargin.cbegin(); it != rankmargin.rankMargin.cend(); ++it)
    {
        double minValueTemp = *std::min_element(it->second.begin(),it->second.end());
        auto minValueTempIt = std::distance(it->second.begin(), std::min_element(it->second.begin(), it->second.end()));
        tempValueStore.push_back(minValueTemp);

        rmtJson[it->first] = { { rankmargin.rankMardinValues[minValueTempIt], minValueTemp } };
    }

    double minValueRank = *std::min_element(tempValueStore.begin(),tempValueStore.end());
    rmtJson["Worst case Rank"] = minValueRank;

    tempValueStore.clear();

    if(DEBUG)
    {
        for(auto it = rankmargin.laneMargin.cbegin(); it != rankmargin.laneMargin.cend(); ++it)
        {
            double minValueTemp = *std::min_element(it->second.begin(),it->second.end());
            auto minValueTempIt = std::distance(it->second.begin(), std::min_element(it->second.begin(), it->second.end()));
            tempValueStore.push_back(minValueTemp);

            rmtJson[it->first] = { { rankmargin.laneMardinValues[minValueTempIt], minValueTemp } };
        }

        double minValueLane = *std::min_element(tempValueStore.begin(),tempValueStore.end());
        rmtJson["Worst case Lane"] = minValueLane;
    }

    output <<  std::setw(4) << rmtJson.dump(2) << std::endl;
}

void printJson()
{
    std::ifstream input("/var/log/rmt.json");
    std::string line;
    nlohmann::json j;
    input >> j;

    for (auto& el : j["N0.C0.D0.R0"].items())
    {
      std::cout << el.value() << '\n';
    }
}

std::string serverName()
{
    std::string board = "board";
    std::string line;
    std::ifstream file;
    file.open("/usr/share/openrack/machine", std::ios::in);

    if (!file.is_open())
    {
        throw std::runtime_error("Unable to open /usr/share/openrack/machine file");
    }

    while (getline(file, line))
    {
        if (line.find("model=") != std::string::npos)
        {
            line.erase(0, 6);
            std::stringstream iss(line);
            iss >> board;
            break;
        }
    }

    return board;
}

static int parseEyeDataValues(std::string& line, std::string mbistKey, mbistEyeDataMargin& mbisteyedatamargin)
{
    bool startCounter = false;
    int cnt = 0;
    std::ifstream input("/var/log/obmc-console.log");

    std::vector<double> marginResults;
    std::vector<std::string> temp;

    while (getline(input, line))
    {
        if(!line.empty())
        {
            if(startCounter)
            {
                line.erase(std::remove(line.begin(), line.end(), '\t'), line.end());
                line.erase(std::remove(line.begin(), line.end(), '.'), line.end());
                line.erase(std::remove(line.begin(), line.end(), '+'), line.end());
                line.erase(std::remove(line.begin(), line.end(), '-'), line.end());
                temp.push_back(line);

                cnt++;
            }

            if (line.find(mbistKey) != std::string::npos)
            {
                startCounter = true;
            }

            if(cnt >= 4)
            {
                startCounter = false;
                cnt = 0;
            }
        }
    }

    if(temp.empty())
    {
        std::cerr << "No valid data found" << "\n";
        return -1;
    }

    for(int i=0; i<temp.size(); i++)
    {
        std::stringstream sss(temp[i]);
        std::string item;

        while (getline(sss, item, ' '))
        {
            unsigned int x = 0;
            std::stringstream ss;

            if (!item.empty() && item != " ")
            {
                ss << std::hex << item;
                ss >> x;

                if(x != 0)
                {
                    marginResults.push_back(static_cast<int>(x));
                }
            }
        }
    }

    //{ "TxV+", "Dq+", "TxV-", "Dq-" };
    std::vector<int> marginResultsWrTxPlus;
    std::vector<int> marginResultsWrDqPlus;
    std::vector<int> marginResultsWrTxMinus;
    std::vector<int> marginResultsWrDqMinus;

    //{ "RxV+", "Dq+", "RxV-", "Dq-" };
    std::vector<int> marginResultsRdRxPlus;
    std::vector<int> marginResultsRdDqPlus;
    std::vector<int> marginResultsRdRxMinus;
    std::vector<int> marginResultsRdDqMinus;

    for(int i = 0; i < marginResults.size(); i++)
    {
        if (mbistKey == "Write Eye Data Margin")
        {
            marginResultsWrTxPlus.push_back(marginResults[i]);
            marginResultsWrDqPlus.push_back(marginResults[i+1]);
            marginResultsWrTxMinus.push_back(marginResults[i+2]);
            marginResultsWrDqMinus.push_back(marginResults[i+3]);
            
            if(DEBUG)
            {
                std::cerr << i << ":Write One +TX: " << marginResults[i] << "\n";
                std::cerr << i+1 << ":Write Two +Dq: " << marginResults[i+1] << "\n";
                std::cerr << i+2 << ":Write Three -Tx: " << marginResults[i+2] << "\n";
                std::cerr << i+3 << ":Write Four -Dq: " << marginResults[i+3] << "\n";
            } 
        }
        else
        {
            if(DEBUG)
            {
                std::cerr << i << ":Read One +TX: " << marginResults[i] << "\n";
                std::cerr << i+1 << ":Read Two +Dq: " << marginResults[i+1] << "\n";
                std::cerr << i+2 << ":Read Three -Tx: " << marginResults[i+2] << "\n";
                std::cerr << i+3 << ":Read Four -Dq: " << marginResults[i+3] << "\n";
            }

            marginResultsRdRxPlus.push_back(marginResults[i]);
            marginResultsRdDqPlus.push_back(marginResults[i+1]);
            marginResultsRdRxMinus.push_back(marginResults[i+2]);
            marginResultsRdDqMinus.push_back(marginResults[i+3]);
        }

        i += 3;
    }

    if (mbistKey == "Write Eye Data Margin")
    {
        mbisteyedatamargin.writeDataEyeMargin["TxV+"] = marginResultsWrTxPlus;
        mbisteyedatamargin.writeDataEyeMargin["WDq+"] = marginResultsWrDqPlus;
        mbisteyedatamargin.writeDataEyeMargin["TxV-"] = marginResultsWrTxMinus;
        mbisteyedatamargin.writeDataEyeMargin["WDq-"] = marginResultsWrDqMinus;

        if(DEBUG)
        {
            for(auto it = mbisteyedatamargin.writeDataEyeMargin.cbegin(); it != mbisteyedatamargin.writeDataEyeMargin.cend(); ++it)
            {
                std::cout << it->first << ": ";
                for (auto i : it->second)
                {
                    std::cout << i << "; ";
                }
                std::cout << "\n";
            }
        }
    }
    else
    {
        mbisteyedatamargin.readDataEyeMargin["RxV+"] = marginResultsRdRxPlus;
        mbisteyedatamargin.readDataEyeMargin["RDq+"] = marginResultsRdDqPlus;
        mbisteyedatamargin.readDataEyeMargin["RxV-"] = marginResultsRdRxMinus;
        mbisteyedatamargin.readDataEyeMargin["RDq-"] = marginResultsRdDqMinus;

        if(DEBUG)
        {
            for(auto it = mbisteyedatamargin.readDataEyeMargin.cbegin(); it != mbisteyedatamargin.readDataEyeMargin.cend(); ++it)
            {
                std::cout << it->first << ": ";
                for (auto i : it->second)
                {
                    std::cout << i << "; ";
                }
                std::cout << "\n";
            }
        }
    }

    input.close();
    return 0;
}

static int getProperty(sdbusplus::bus::bus& bus, const std::string& path,
                 const std::string& property, std::string& value, const std::string service, const std::string interface)
{
    auto method = bus.new_method_call(service.c_str(), path.c_str(), PROPERTY_INTERFACE, "Get");
    method.append(interface.c_str(),property);
    auto reply=bus.call(method);
    if (reply.is_method_error())
    {
        std::cerr << "Error looking up services, PATH=" << interface << "\n";
        return -1;
    }

    std::variant<std::string> valuetmp;
    try
    {
        reply.read(valuetmp);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::cerr << "Failed to get pattern string for match process\n";
        return -1;
    }

    value = std::get<std::string>(valuetmp);
    return 0;
}

bool is_empty(std::ifstream& pFile)
{
    return pFile.peek() == std::ifstream::traits_type::eof();
}

static int startRMTparsingService()
{
    std::ifstream input("/var/log/obmc-console.log");
    std::ofstream output("/var/log/rmt-output.log");
    std::vector<std::string> logResoultsValues;
    std::string line;

    std::string rmtStart = "START_BSSA_RMT";
    std::string rmtStop = "STOP_BSSA_RMT";

    std::string mbistKey;
    std::vector<std::string> eyeDataMargin;

    std::regex regexpRank("N[0-9].C[0-9].D[0-9].R[0-9]");
    std::regex regexpLane("N[0-9].C[0-9].D[0-9].R[0-9].L[0-9][0-9]");

    rankMargin rankmargin;
    mbistEyeDataMargin mbisteyedatamargin;

    bool startCounter = false;
    bool matchReadyFlag = false;

    if(input.fail())
    {
        std::cerr << "input file obmc-console.log is incorrect!" << "\n";
        return -1;
    }

    if(output.fail())
    {
        std::cerr << "output file rmt-output.log is incorrect!" << "\n";
        return -1;
    }

    std::string board = "mzb2";

    board = serverName();

    int rc;

    if(board == "mzb2" || board == "mz81")
    {
        std::ofstream output("/var/log/rmt.json");

        nlohmann::ordered_json rmtJson;
        std::vector<double> tempValueStore;

        mbistKey = "Write Eye Data Margin";
        rc = parseEyeDataValues(line, mbistKey, mbisteyedatamargin);

        rmtJson["Write"] = { "Write Eye Data Margin Channel: [0-7], ChipSelect: [0-1] : Vref : DqDelay" };

        if(rc < 0)
        {
            std::cerr << "Cant find any of write Eye Data Margin" << "\n";
            output.close();
            return -1;
        }

        tempValueStore.clear();

        for(auto it = mbisteyedatamargin.writeDataEyeMargin.cbegin(); it != mbisteyedatamargin.writeDataEyeMargin.cend(); ++it)
        {
            double minValueTemp = *std::min_element(it->second.begin(),it->second.end());
            rmtJson[it->first] = { minValueTemp };
            tempValueStore.push_back(minValueTemp);
        }

        double minValueWrite = *std::min_element(tempValueStore.begin(),tempValueStore.end());
        rmtJson["Worst case Write"] = minValueWrite;

        tempValueStore.clear();

        mbistKey = "Read Eye Data Margin";
        rc = parseEyeDataValues(line, mbistKey, mbisteyedatamargin);

        rmtJson["Read"] = { "Read Eye Data Margin Channel: [0-7], ChipSelect: [0-1] : Vref : DqDelay" };

        if(rc < 0)
        {
            std::cerr << "Cant find any of read Eye Data Margin" << "\n";
            output.close();
            return -1;
        }

        for(auto it = mbisteyedatamargin.readDataEyeMargin.cbegin(); it != mbisteyedatamargin.readDataEyeMargin.cend(); ++it)
        {
            double minValueTemp = *std::min_element(it->second.begin(), it->second.end());
            rmtJson[it->first] = { minValueTemp };
            tempValueStore.push_back(minValueTemp);
        }

        double minValueRead = *std::min_element(tempValueStore.begin(),tempValueStore.end());
        rmtJson["Worst case Read"] = minValueRead;

        output <<  std::setw(4) << rmtJson.dump(2) << std::endl;

        output.close();
    }
    else
    {
        while (getline(input, line))
        {
            if(startCounter)
            {
                output << line << "\n";
            }

            if(line.erase(line.find_last_not_of(" \n\r\t")+1) == rmtStart)
            {
                output << "START_BSSA_RMT";
                startCounter = true;
            }

            if(line.erase(line.find_last_not_of(" \n\r\t")+1) == rmtStop)
            {
                startCounter = false;
                break;
            }
        }

        input.close();
        output.close();

        std::ifstream pfile("/var/log/rmt-output.log");
        std::cerr << "Intel MB found, launching Rank Marging Tool" << "\n";
        if (is_empty(pfile))
        {
            output << "No BSSA_RMT found!";
            std::cerr << "Looking for /var/log/rmt-output.log, file empty" << "\n";
            pfile.close();
            return -1;
        }

        pfile.close();

        findRankMarginResults(rankmargin, matchReadyFlag, regexpRank);

        if(DEBUG)
        {
            printMap(rankmargin.rankMargin);
        }

        findLaneMarginResults(rankmargin, matchReadyFlag, regexpLane);

        if(DEBUG)
        {
            printMap(rankmargin.laneMargin);
        }

        makeJson(rankmargin);

        if(DEBUG)
        {
            printJson();
        }
    }

    return 0;
}

static int registerPostCompleteServiceChangeCallback()
{
    constexpr const char* CHASSIS_BUS = "xyz.openbmc_project.Chassis.Buttons";
    constexpr const char* CHASSIS_PATH = "/xyz/openbmc_project/state/os";
    constexpr const char* CHASSIS_PROPERTY = "OperatingSystemState";
    constexpr const char* CHASSIS_INTF = "xyz.openbmc_project.State.OperatingSystem.Status";

    boost::asio::io_service io;

    auto bus = sdbusplus::bus::new_default();
    static auto conn = std::make_shared<sdbusplus::asio::connection>(io);

    int rc;
    std::string responseData;
    rc = getProperty(bus, CHASSIS_PATH, CHASSIS_PROPERTY, responseData, CHASSIS_BUS, CHASSIS_INTF);

    if(rc < 0)
    {
        std::cerr << "Can't get property" << "\n";
        return -1;
    }

    std::cerr << "OS state : " << responseData << "\n";

    sdbusplus::bus::match::match powerMatch(
        static_cast<sdbusplus::bus::bus &>(*conn),
        "type='signal',member='PropertiesChanged',path='" +
            std::string(CHASSIS_PATH) + "',arg0='" +
            std::string(CHASSIS_INTF) + "'",
        [CHASSIS_PROPERTY](sdbusplus::message::message &message) {
            std::string objectName;
            boost::container::flat_map<std::string, std::variant<std::string>>
                values;
            message.read(objectName, values);
            auto findState = values.find(CHASSIS_PROPERTY);
            if (findState != values.end())
            {
                if (boost::ends_with(std::get<std::string>(findState->second),
                                     "Standby"))
                {
                    std::cerr << "Found power match state" << "\n";
                    sleep(60);
                    int rc = startRMTparsingService();
                    if(rc < 0)
                    {
                        std::cerr << "Failed to launch RMT Parsing" << "\n";
                    }
                }
            }
        });
    
    io.run();
    return 0;
}

int main(int argc, char* argv[])
{
    std::string value;
    CLI::App app("*** RMT parsing CLI start ***");
    app.add_option("-v,--verbose", DEBUG, "print debug output");
    app.add_option("-f,--force", value, "force run RMT parser");
    CLI11_PARSE(app, argc, argv);

    //Just in case
    if (value == "run")
    {
        int rc = startRMTparsingService();
        if(rc < 0)
        {
            std::cerr << "Failed to launch RMT Parsing" << "\n";
        }
        return 0;
    }

    int rc;
    rc = registerPostCompleteServiceChangeCallback();
    
    if(rc < 0)
    {
        std::cerr << "Can't match Post Complete property" << "\n";
        return -1;
    }

    return 0;
}
