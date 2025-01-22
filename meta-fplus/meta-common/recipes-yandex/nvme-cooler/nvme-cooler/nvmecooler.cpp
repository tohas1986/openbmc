/*
 *  nvmecooler.cpp
 *
 *  Created on: May 18, 2021
 *  Author: Nikita Vedeneev
 *  Company: Yandex LLC
 */

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <regex>
#include <nlohmann/json.hpp>
#include <sstream>
#include <map>
#include <iomanip>

#include "nvmecooler.hpp"
#include "smbus.hpp"
#include "i2c.h"
#include <mutex>
#include <CLI/CLI.hpp>

extern "C"
{
#include <i2c/smbus.h>
#include <linux/i2c-dev.h>
}

bool DEBUG = false;

static constexpr const int TEMPERATURE_SENSOR_FAILURE = 0x81;

static constexpr const uint8_t COMMAND_CODE_0 = 0;
static constexpr const uint8_t COMMAND_CODE_8 = 8;

static constexpr const uint8_t CODE_0_LENGTH = 8;
static constexpr const uint8_t CODE_8_LENGTH = 23;

static constexpr int SERIALNUMBER_START_INDEX = 3;
static constexpr int SERIALNUMBER_END_INDEX = 23;
static constexpr int MODELNUMBER_START_INDEX = 46;
static constexpr int MODELNUMBER_END_INDEX = 85;

static std::map<std::string, std::string> map_vendor = {{"80 86", "INTEL"},
                                                        {"14 4d", "SAMSUNG"}};

std::string intToHex(int input)
{
    std::stringstream tmp;
    tmp << std::hex << input;

    return tmp.str();
}

void parseNVMETable(NVMETableStruct& nvmetable)
{
    std::ifstream nvmeCoolingTable("/usr/share/nvme-cooler/nvme-pn-table.json");
    nlohmann::json j;
    nvmeCoolingTable >> j;

    for (auto& el : j.items())
    {
        if(DEBUG)
        {
            std::cout << std::setw(4) << el.key() << "::" << el.value() << '\n';
        }
        nvmetable.Models[el.key()] = el.value();
    }
}

std::string ReadNVMEFru(int busID)
{
    NVMeData nvmeData;

    nvmeData.present = true;
    nvmeData.vendor = "NOVENDOR";
    nvmeData.serialNumber = "";
    nvmeData.modelNumber = "NOMODEL";
    nvmeData.smartWarnings = "";
    nvmeData.statusFlags = "";
    nvmeData.driveLifeUsed = "";
    nvmeData.sensorValue = static_cast<int8_t>(TEMPERATURE_SENSOR_FAILURE);
    nvmeData.wcTemp = 0;

    phosphor::smbus::Smbus smbus;

    unsigned char rsp_data_command_0[I2C_DATA_MAX] = {0};
    unsigned char rsp_data_command_8[I2C_DATA_MAX] = {0};

    uint8_t tx_data = COMMAND_CODE_0;

    auto init = smbus.smbusInit(busID);

    static std::unordered_map<int, bool> isErrorSmbus;

    if (init == -1)
    {
        if (isErrorSmbus[busID] != true)
        {
            if(DEBUG)std::cerr << "smbusInit fail!" << "\n";
            isErrorSmbus[busID] = true;
        }

        nvmeData.present = false;

        exit -1;
    }

    auto res_int = smbus.SendSmbusRWCmdRAW(busID, NVME_SSD_SLAVE_ADDRESS,
                                           &tx_data, sizeof(tx_data),
                                           rsp_data_command_0, CODE_0_LENGTH);

    if (res_int < 0)
    {
        if (isErrorSmbus[busID] != true)
        {
            if(DEBUG)std::cerr << "Send command code 0 fail!";
            isErrorSmbus[busID] = true;
        }

        smbus.smbusClose(busID);
        nvmeData.present = false;
        exit -1;
    }

    nvmeData.statusFlags = intToHex(rsp_data_command_0[1]);
    nvmeData.smartWarnings = intToHex(rsp_data_command_0[2]);
    nvmeData.driveLifeUsed = intToHex(rsp_data_command_0[4]);
    nvmeData.sensorValue = static_cast<int8_t>(rsp_data_command_0[3]);
    nvmeData.wcTemp = static_cast<int8_t>(rsp_data_command_0[5]);

    isErrorSmbus[busID] = false;

    if(DEBUG)
    {
        std::cerr << "statusFlags: " << nvmeData.statusFlags << "\n";
        std::cerr << "smartWarnings: " << nvmeData.smartWarnings << "\n";
        std::cerr << "driveLifeUsed: " << nvmeData.driveLifeUsed << "\n";
        std::cerr << "sensorValue: " << +nvmeData.sensorValue << "\n";
        std::cerr << "wcTemp: " << +nvmeData.wcTemp << "\n";
    }

    tx_data = COMMAND_CODE_8;

    res_int = smbus.SendSmbusRWCmdRAW(busID, NVME_SSD_SLAVE_ADDRESS, &tx_data,
                                      sizeof(tx_data), rsp_data_command_8,
                                      CODE_8_LENGTH);

    if (res_int < 0)
    {
        if (isErrorSmbus[busID] != true)
        {
            if(DEBUG)std::cerr << "Send command code 8 fail!" << "\n";
            isErrorSmbus[busID] = true;
        }

        smbus.smbusClose(busID);
        exit -1;
    }

    nvmeData.vendor =
        intToHex(rsp_data_command_8[1]) + " " + intToHex(rsp_data_command_8[2]);

    for (auto iter = map_vendor.begin(); iter != map_vendor.end(); iter++)
    {
        if (iter->first == nvmeData.vendor)
        {
            nvmeData.vendor = iter->second;

            if(DEBUG)std::cerr << "Vendor: " << nvmeData.vendor << "\n";
            break;
        }
    }

    if(nvmeData.vendor == "INTEL")
    {
        nvmeData.modelNumber = "";

        uint8_t data = 0x2e;
        uint8_t CODE_LENGTH = 19;
        uint8_t slaveaddr = NVME_SSD_VPD_SLAVE_ADDRESS;
        unsigned char rsp_data_command[CODE_LENGTH] = {0};

        res_int = smbus.SendSmbusRWCmdRAW(busID, slaveaddr, &data,
                                        sizeof(data), rsp_data_command, CODE_LENGTH);

        if (res_int < 0)
        {
            if (isErrorSmbus[busID] != true)
            {
                if(DEBUG)std::cerr << "Send command code 8 fail!" << "\n";
                isErrorSmbus[busID] = true;
            }

            smbus.smbusClose(busID);
            exit -1;
        }

        if(DEBUG)
        {
            for(uint8_t rsp : rsp_data_command)
            {
                std::cerr << std::hex << std::setw(2) << +rsp << " ";
            }
        }

        for (int i = 0; i < CODE_LENGTH; i++)
        {
            if (rsp_data_command[i] != ' ')
                nvmeData.modelNumber += static_cast<char>(rsp_data_command[i]);
        }

        if (nvmeData.modelNumber.substr(0, 5) == "INTEL")
        {
            nvmeData.modelNumber.erase(0, 5);
        }
    }

    if (nvmeData.vendor == "SAMSUNG")
    {
        nvmeData.modelNumber = "";

        unsigned char rsp_data_vpd[I2C_DATA_MAX] = {0};
        uint8_t rx_len = 37; //(MODELNUMBER_END_INDEX - MODELNUMBER_START_INDEX);
        tx_data = 32; MODELNUMBER_START_INDEX;

        auto res_int =
            smbus.SendSmbusRWCmdRAW(busID, NVME_SSD_VPD_SLAVE_ADDRESS, &tx_data,
                                    sizeof(tx_data), rsp_data_vpd, rx_len);

        if (res_int < 0)
        {
            if (isErrorSmbus[busID] != true)
            {
                if(DEBUG)std::cerr << "Send command code VPD fail!" << "\n";
                isErrorSmbus[busID] = true;
            }

            smbus.smbusClose(busID);
            exit -1;
        }

        for (int i = 0; i < rx_len; i++)
        {
            if (rsp_data_vpd[i] != ' ')
                nvmeData.modelNumber += static_cast<char>(rsp_data_vpd[i]);
        }

        if (nvmeData.modelNumber.substr(0, nvmeData.vendor.size()) == "SAMSUNG")
            nvmeData.modelNumber.erase(0, nvmeData.vendor.size());
    }

    nvmeData.modelNumber.erase(remove_if(nvmeData.modelNumber.begin(), nvmeData.modelNumber.end(), isspace), nvmeData.modelNumber.end());
    if(DEBUG)std::cerr << "NVME Model: "<< nvmeData.modelNumber << "\n";

    smbus.smbusClose(busID);

    isErrorSmbus[busID] = false;

    return nvmeData.modelNumber;
}

uint8_t CompareNVMEModels(std::vector<std::string>& nvmeModelFromFRU,
                            NVMETableStruct& nvmetable)
{
    std::vector<uint8_t> tempValue;
    uint8_t setpoint = 75;

    for(auto& model : nvmeModelFromFRU)
    {
        for(auto& elem : nvmetable.Models)
        {
            if(DEBUG)
            {
                std::cerr << model << " " << model.size() << "\n";
                std::cerr << elem.first << " " << elem.first.size() << "\n";
            }
            std::string str = (std::string)model;
            str.resize(elem.first.size());

            if ( (std::string)elem.first == str ) 
            {
                if(DEBUG)
                {
                    std::cout << "NVME model: " << str << " founded, set update cooling setpoint true" << "\n";
                }
                tempValue.push_back(nvmetable.Models.at(str));
            }
            else
            {
                if(DEBUG)
                {
                    std::cout << "NVME model: " << model << " not found in pre-config json file" << "\n";
                }
            }
        }
    }

    if(tempValue.size() > 0)
    {
        setpoint = *min_element(tempValue.begin(), tempValue.end());
    }

    return setpoint;
}

void restartDecision(uint8_t& oldCoolingSetPoint, uint8_t& setpoint)
{
    if(oldCoolingSetPoint == setpoint)
    {
        std::cerr << "No need to restart PID control service" << "\n";
    }
    else
    {
        std::cerr << "Need to restart PID control service" << "\n";

        char command[100];
        std::stringstream cmd;
        std::string cmd_str;

        cmd << "systemctl restart phosphor-pid-control.service";
        std::cerr << cmd.str() << "\n";
        cmd_str = cmd.str();

        try
        {
          memset(command,0,sizeof(command));
          std::copy(cmd_str.begin(), cmd_str.end(), command);
          system(command);
        }
        catch (std::system_error&)
        {
            std::cerr << "Error can't restrt PID control service" << "\n";
            exit -1;
        }
    }
}

void changeNVMEPIDSettings(uint8_t& setpoint)
{
    uint8_t oldCoolingSetPoint = 0;

    std::ifstream pidConfigFile("/usr/share/swampd/config.json");
    nlohmann::json j;
    pidConfigFile >> j;

    JsonStruct jsonstruct;

    auto zones = j["zones"];
    std::ofstream outputPIDconfigFile("/usr/share/swampd/config.json");

    for (auto &zone : zones) {
        auto pids = zone["pids"];

        for (auto &name : pids) {
            jsonstruct.name = name["name"];

            if (name["name"] == "NVME") {
                auto pid = name["pid"];
                auto reading = pid.find("reading");

                std::cerr << reading.key() << ": " << reading.value().at("1")
                        << "\n";

                oldCoolingSetPoint = reading.value().at("1");

                if (oldCoolingSetPoint == setpoint) {
                    reading.value().at("1") = oldCoolingSetPoint;
                } else {
                    reading.value().at("1") = setpoint;
                }

                std::cerr << reading.key() << ": " << reading.value().at("1")
                        << "\n";

                pid["reading"] = reading.value();

                name["pid"] = pid;
            }
        }
        zone["pids"] = pids;
    }

    j["zones"] = zones;

    if(DEBUG)
    {
        std::cerr << "Updating cooling setpoint from: " << +oldCoolingSetPoint
            << " to " << +setpoint << "\n";
    }
    outputPIDconfigFile << std::setw(4) << j;

    outputPIDconfigFile.close();
    pidConfigFile.close();

    //Need to care about PID control service restart
    restartDecision(oldCoolingSetPoint, setpoint);
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

int main(int argc, char* argv[])
{
    CLI::App app("NVME Cooling service DEBUG mode: On");
    app.add_option("-v,--verbose", DEBUG, "print debug output");
    CLI11_PARSE(app, argc, argv);

    uint8_t setpoint =  75;
    int busID;
    std::vector<std::string> nvmeModelFromFRU;
    bool needChangeSetPoint = false;

    NVMETableStruct nvmetable;

    //Get NVME models from json config file
    parseNVMETable(nvmetable);

    /*Get NVME models from FRU directly
    *
    * BUS ID:
    *   - AI1  = {17, 18, 19, 20, 21, 22};
    *   - MY62 = {16, 17, 18, 19, 20, 21};
    *   - MZB2 FRONT  = {16, 17, 18, 19, 20, 21};
    *   - MZB2 MIDDLE = {32, 33, 34, 35, 36, 36};
    */

    if(serverName() == "ai1")
    {
        std::vector<int> busIDs = {17, 18, 19, 20, 21, 22};

        for(int busID : busIDs)
        {
            nvmeModelFromFRU.push_back(ReadNVMEFru(busID));
        }
    }
    else if(serverName() == "my62")
    {
        std::vector<int> busIDs = {16, 17, 18, 19, 20, 21};

        for(int busID : busIDs)
        {
            nvmeModelFromFRU.push_back(ReadNVMEFru(busID));
        }
    }
    else if(serverName() == "mzb2")
    {
        std::vector<int> busIDs = {16, 17, 18, 19, 20, 21, 32, 33, 34, 35, 36, 37};

        for(int busID : busIDs)
        {
            nvmeModelFromFRU.push_back(ReadNVMEFru(busID));
        }
    }

    //Compare NVME models from config file with NVME models from FRU
    setpoint = CompareNVMEModels(nvmeModelFromFRU, nvmetable);

    //Change PID settings for currenct NVME drive if needed
    changeNVMEPIDSettings(setpoint);

    return 0;
}
