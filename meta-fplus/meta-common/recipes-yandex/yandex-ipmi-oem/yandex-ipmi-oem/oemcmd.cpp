#include "oemcmd.hpp"
#include "file.hpp"
#include <ipmid/api.h>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <systemd/sd-bus.h>
#include <endian.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>

#include <algorithm>

#include <linux/types.h>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/vtable.hpp>
#include <sdbusplus/server/interface.hpp>
#include <sdbusplus/timer.hpp>
#include <gpiod.hpp>

#include <sdbusplus/asio/connection.hpp>
#include <boost/asio.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <ipmid/api.hpp>
#include <ipmid/utils.hpp>
#include <phosphor-logging/log.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message/types.hpp>
#include <peci.h>

#include "xyz/openbmc_project/Control/Power/RestorePolicy/server.hpp"

#define FSC_SERVICE "xyz.openbmc_project.EntityManager"
#define FSC_OBJECTPATH "/xyz/openbmc_project/inventory/system/board/s7106_Baseboard/Pid_"
#define PID_INTERFACE "xyz.openbmc_project.Configuration.Pid.Zone"
#define PROPERTY_INTERFACE "org.freedesktop.DBus.Properties"

#define CHASSIS_BUS "xyz.openbmc_project.State.Chassis"
#define CHASSIS_PATH "/xyz/openbmc_project/state/os"
#define CHASSIS_PROPERTY "OperatingSystemState"
#define CHASSIS_INTF "xyz.openbmc_project.State.OperatingSystem.Status"

#include <filesystem>
#include "nlohmann/json.hpp"

extern "C"
{
#include <i2c/smbus.h>
//#include <linux/i2c-dev.h>
}

using phosphor::logging::level;
using phosphor::logging::log;

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Control::Power::server;
using primarycode_t = uint64_t;
using secondarycode_t = std::vector<uint8_t>;
using postcode_t = std::tuple<primarycode_t, secondarycode_t>;

static constexpr const uint32_t mdr2SMBaseAddress = 0x9FF00000; // Base address of VGA shared memory

static const size_t retPostCodeLen = 512;

namespace fs = std::filesystem;

namespace ipmi
{

enum class LanParam : uint8_t
{
    IP = 2,
    MAC = 3,
    IPV6 = 4,
};

namespace network
{

constexpr auto ROOT = "/xyz/openbmc_project/network";
constexpr auto SERVICE = "xyz.openbmc_project.Network";
constexpr auto IPV4_TYPE = "ipv4";
constexpr auto IPV6_TYPE = "ipv6";
constexpr auto IPV4_PREFIX = "169.254";
constexpr auto IPV6_PREFIX = "fe80";
constexpr auto IP_INTERFACE = "xyz.openbmc_project.Network.IP";
constexpr auto MAC_INTERFACE = "xyz.openbmc_project.Network.MACAddress";

constexpr auto MDR_ROOT = "/xyz/openbmc_project/inventory/system/chassis/motherboard";
constexpr auto MDR_SERVICE = "xyz.openbmc_project.Smbios.MDR_V2";
constexpr auto BIOS_INTERFACE = "xyz.openbmc_project.Inventory.Decorator.Revision";
constexpr auto ETH_INTERFACE = "xyz.openbmc_project.Inventory.Item.Ethernet";

constexpr auto FRU_INTERFACE = "xyz.openbmc_project.FruDevice";
constexpr auto FRU_ROOT = "/xyz/openbmc_project/FruDevice/";

bool isLinkLocalIP(const std::string &address)
{
    return address.find(IPV4_PREFIX) == 0 || address.find(IPV6_PREFIX) == 0;
}
DbusObjectInfo getIPObject(sdbusplus::bus::bus &bus,
                           const std::string &interface,
                           const std::string &serviceRoot,
                           const std::string &match)
{
    auto objectTree = getAllDbusObjects(bus, serviceRoot, interface, match);

    if (objectTree.empty())
    {
        log<level::ERR>("No Object has implemented the IP interface",
                        entry("INTERFACE=%s", interface.c_str()));
    }

    DbusObjectInfo objectInfo;

    for (auto &object : objectTree)
    {
        auto variant =
            ipmi::getDbusProperty(bus, object.second.begin()->first,
                                  object.first, IP_INTERFACE, "Address");

        objectInfo = std::make_pair(object.first, object.second.begin()->first);
        // if LinkLocalIP found look for Non-LinkLocalIP
        if (isLinkLocalIP(std::get<std::string>(variant)))
        {
            continue;
        }
        else
        {
            break;
        }
    }
    return objectInfo;
}

DbusObjectInfo getIPLocalObject(sdbusplus::bus::bus& bus,
                           const std::string& interface,
                           const std::string& serviceRoot,
                           const std::string& match)
{
    auto objectTree = getAllDbusObjects(bus, serviceRoot, interface, match);

    if (objectTree.empty())
    {
        log<level::ERR>("No Object has implemented Local IP interface",
                        entry("INTERFACE=%s", interface.c_str()));
    }

    DbusObjectInfo objectInfo;

    for (auto& object : objectTree)
    {
        auto variant =
            ipmi::getDbusProperty(bus, object.second.begin()->first,
                                  object.first, IP_INTERFACE, "Address");

        objectInfo = std::make_pair(object.first, object.second.begin()->first);

        // if LinkLocalIP found - break
        if (isLinkLocalIP(std::get<std::string>(variant)))
        {
            break;
        }
        else
        {
            continue;
        }
    }
    return objectInfo;
}

} // namespace network

static int getProperty(sdbusplus::bus::bus& bus, const std::string& path,
                 const std::string& property, double& value, const std::string service, const std::string interface)
{
    auto method = bus.new_method_call(service.c_str(), path.c_str(), PROPERTY_INTERFACE, "Get");
    method.append(interface.c_str(),property);
    auto reply=bus.call(method);
    if (reply.is_method_error())
    {
        std::cerr << "Error looking up services, PATH=" << interface << "\n";
        return -1;
    }

    std::variant<double> valuetmp;
    try
    {
        reply.read(valuetmp);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::cerr << "Failed to get pattern string for match process\n";
        return -1;
    }

    value = std::get<double>(valuetmp);
    return 0;
}

static void setProperty(sdbusplus::bus::bus& bus, const std::string& path,
                 const std::string& property, const double value)
{
    auto method = bus.new_method_call(FSC_SERVICE, path.c_str(),
                                      PROPERTY_INTERFACE, "Set");
    method.append(PID_INTERFACE, property, std::variant<double>(value));
    bus.call_noreply(method);

    return;
}

void register_netfn_yndx_oem() __attribute__((constructor));

int execmd(char* cmd,char* result) {
        char buffer[128];
        FILE* pipe = popen(cmd, "r");
        if (!pipe)
                return -1;

        while(!feof(pipe)) {
                if(fgets(buffer, 128, pipe)){
                        strcat(result,buffer);
                }
        }
        pclose(pipe);
        return 0;
}

std::unique_ptr<phosphor::Timer> clrCmosTimer = nullptr;
#define AST_GPIO_F1_PIN 41
bool clrCmos()
{
#if 0
    //todo : move to device tree by assigned name
    auto line = gpiod::find_line("CLR_CMOS");

    if (!line)
    {
        return false;
    }
#endif
    auto chip = gpiod::chip("gpiochip0");
    auto line = chip.get_line(AST_GPIO_F1_PIN);
    if (!line)
    {
        std::cerr << "Error requesting gpio AST_GPIO_F1_PIN\n";
        return false;
    }
#if 0
    auto dir = line.direction();
    std::cerr << "direction :" <<dir<<"\n";
#endif
    int value = 1;

    try
    {
        line.request({"ipmid", gpiod::line_request::DIRECTION_OUTPUT,0}, value);
    }
    catch (std::system_error&)
    {
        std::cerr << "Error requesting gpio\n";
        return false;
    }

    line.set_value(0);
    sleep(3);
    line.set_value(1);

    line.release();
    return true;

}
void createTimer()
{
    if (clrCmosTimer == nullptr)
    {
        clrCmosTimer = std::make_unique<phosphor::Timer>(clrCmos);
    }
}


/*
    NetFun: 0x3e
    Cmd : 0x3a
    Request:
*/
ipmi_ret_t ipmiOpmaClearCmos(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                              ipmi_request_t request, ipmi_response_t response,
                              ipmi_data_len_t data_len, ipmi_context_t context)
{
    ipmi_ret_t ipmi_rc = IPMI_CC_OK;

    //todo, check pgood status
    createTimer();
    if (clrCmosTimer == nullptr)
    {
        return IPMI_CC_RESPONSE_ERROR;
    }

    if(clrCmosTimer->isRunning())
    {
        return IPMI_CC_RESPONSE_ERROR;
    }
    clrCmosTimer->start(std::chrono::duration_cast<std::chrono::microseconds>(
    std::chrono::seconds(0)));

    return ipmi_rc;

}

//===============================================================
/* Set Fan Control Enable Command
NetFun: 0x2E
Cmd : 0x06
Request:
        Byte 1-3 : Tyan Manufactures ID (FD 19 00)
        Byte 4 :  0h - Fan Control Disable
                  1h - Fan Control Enable
                  ffh - Get Current Fan Control Status
Response:
        Byte 1 : Completion Code
        Byte 2-4 : Tyan Manufactures ID
        Byte (5) : Current Fan Control Status , present if FFh passed to Enable Fan Control in Request
*/
ipmi::RspType<uint8_t> ipmi_tyan_ManufactureMode(uint8_t mode)
{
    std::fstream file;
    int rc=0;
    char command[100];
    char FSCStatus[100];
    uint8_t currentStatus;
    char Object[100];
    auto bus = sdbusplus::bus::new_default();

    if (mode == 0)
    {
        // Disable Fan Control
        memset(command,0,sizeof(command));
        sprintf(command, "systemctl stop --no-block phosphor-pid-control.service");
        rc = system(command);

        memset(command,0,sizeof(command));
        sprintf(command, "echo 0 > /usr/sbin/fsc");
        system(command);

        // Set all fan to Full Duty
        for(size_t i = 1; i <= 5; i++)
        {
            memset(command,0,sizeof(command));
            sprintf(command, "echo 255 > /sys/class/hwmon/hwmon0/pwm%d",i);
            rc = system(command);
        }
    }
    else if (mode == 1)
    {
        // Enable Fan Control
        memset(command,0,sizeof(command));
        sprintf(command, "systemctl start --no-block phosphor-pid-control.service");
        rc = system(command);

        memset(command,0,sizeof(command));
        sprintf(command, "echo 1 > /usr/sbin/fsc");
        system(command);

        //Set Floor Duty Cycle control by sensors
        for(size_t i = 1; i <= 5; i++)
        {
            memset(Object,0,sizeof(Object));
            snprintf(Object,sizeof(Object),"%s%d",FSC_OBJECTPATH,i);
            setProperty(bus,Object,"CommandSet",0);
        }
    }
    else if (mode == 0xff)
    {
        // Get Current Fan Control Status
        file.open("/usr/sbin/fsc",std::ios::in);
        if(!file)
        {
            memset(command,0,sizeof(command));
            sprintf(command, "touch /usr/sbin/fsc");
            system(command);

            memset(command,0,sizeof(command));
            sprintf(command, "echo 1 > /usr/sbin/fsc");
            system(command);

            currentStatus=1;
        }
        else
        {
            file.read(FSCStatus,sizeof(FSCStatus));
            currentStatus = strtol(FSCStatus,NULL,16);
        }
        file.close();
        return ipmi::responseSuccess(currentStatus);
    }
    else
    {
        return ipmi::responseUnspecifiedError();
    }

    if (rc != 0)
    {
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess();
}
//===============================================================

/* Set Fan Control PWM Duty Command
NetFun: 0x2E
Cmd : 0x05
Request:
        Byte 1-3 : Tyan Manufactures ID (FD 19 00)
        Byte 4 : PWM ID ( 0h-PWM1 , ... , 4h-PWM5 )
        Byte 5 : Duty Cycle
            0-64h - manual control duty cycle (0%-100%)
            FEh - Get current duty cycle
            FFh - Return to automatic control
Response:
        Byte 1 : Completion Code
        Byte 2-4 : Tyan Manufactures ID
        Byte (5) : Current Duty Cycle , present if 0xFE passed to Duty Cycle in Request
*/
ipmi::RspType<uint8_t> ipmi_tyan_FanPwmDuty(uint8_t pwmId, uint8_t duty)
{
    std::fstream file;
    int rc=0;
    char command[100];
    char temp[50];
    uint8_t responseDuty;
    uint8_t pwmValue = 0;

    if (duty == 0xfe)
    {
        // Get current duty cycle
        switch (pwmId)
        {
            case 0:
                    memset(command,0,sizeof(command));
                    sprintf(command, "cat /sys/class/hwmon/hwmon0/pwm1");
                    break;
            case 1:
                    memset(command,0,sizeof(command));
                    sprintf(command, "cat /sys/class/hwmon/hwmon0/pwm2");
                    break;
            case 2:
                    memset(command,0,sizeof(command));
                    sprintf(command, "cat /sys/class/hwmon/hwmon0/pwm3");
                    break;
            case 3:
                    memset(command,0,sizeof(command));
                    sprintf(command, "cat /sys/class/hwmon/hwmon0/pwm4");
                    break;
            case 4:
                    memset(command,0,sizeof(command));
                    sprintf(command, "cat /sys/class/hwmon/hwmon0/pwm5");
                    break;
            default:
                    return ipmi::responseParmOutOfRange();
        }

        memset(temp, 0, sizeof(temp));
        rc = execmd((char *)command, temp);

        if (rc != 0)
        {
            return ipmi::responseUnspecifiedError();
        }

        pwmValue = strtol(temp,NULL,10);
        responseDuty = pwmValue*100/255;
        return ipmi::responseSuccess(responseDuty);
    }
    else if (duty == 0xff)
    {
        // Return to automatic control
        memset(command,0,sizeof(command));
        sprintf(command, "systemctl start --no-block phosphor-pid-control.service");
        rc = system(command);

        memset(command,0,sizeof(command));
        sprintf(command, "echo 1 > /usr/sbin/fsc");
        system(command);
    }
    else if (duty <= 0x64)
    {
        memset(command,0,sizeof(command));
        sprintf(command, "systemctl stop --no-block phosphor-pid-control.service");
        rc = system(command);

        memset(command,0,sizeof(command));
        sprintf(command, "echo 0 > /usr/sbin/fsc");
        system(command);

        // control duty cycle (0%-100%)
        pwmValue = duty*255/100;

        switch (pwmId)
        {
            case 0:
                    memset(command,0,sizeof(command));
                    sprintf(command, "echo %d > /sys/class/hwmon/hwmon0/pwm1", pwmValue);
                    break;
            case 1:
                    memset(command,0,sizeof(command));
                    sprintf(command, "echo %d > /sys/class/hwmon/hwmon0/pwm2", pwmValue);
                    break;
            case 2:
                    memset(command,0,sizeof(command));
                    sprintf(command, "echo %d > /sys/class/hwmon/hwmon0/pwm3", pwmValue);
                    break;
            case 3:
                    memset(command,0,sizeof(command));
                    sprintf(command, "echo %d > /sys/class/hwmon/hwmon0/pwm4", pwmValue);
                    break;
            case 4:
                    memset(command,0,sizeof(command));
                    sprintf(command, "echo %d > /sys/class/hwmon/hwmon0/pwm5", pwmValue);
                    break;
            default:
                    return ipmi::responseParmOutOfRange();
        }
        rc = system(command);
    }
    else
    {
        return ipmi::responseParmOutOfRange();
    }

    if (rc != 0)
    {
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess();
}
//===============================================================
/* Set Floor Duty Command
NetFun: 0x2E
Cmd : 0x07
Request:
        Byte 1-3 : Tyan Manufactures ID (FD 19 00)
        Byte 4 :  0h~64h - Floor Duty cycle
                     ffh - Get Current Floor Duty Cycle
Response:
        Byte 1 : Completion Code
        Byte 2-4 : Tyan Manufactures ID
        Byte (5) : Current Floor Duty Cycle , present if FFh passed to Byte4 in Request
            [7] : Floor Duty Cycle control source
                0b - by sensor.
                1b - by command.
            [6:0] : Floor Duty cycle.
*/
ipmi::RspType<uint8_t> ipmi_tyan_FloorDuty(uint8_t floorDuty)
{
    int rc=0;
    char Object[100];
    double responseData;
    uint8_t currentFloorDuty;
    uint8_t controlSource;

    auto bus = sdbusplus::bus::new_default();

    if (floorDuty <= 0x64)
    {
        //Set Floor Duty Cycle
        for(size_t i = 1; i <= 5; i++)
        {
            memset(Object,0,sizeof(Object));
            snprintf(Object,sizeof(Object),"%s%d",FSC_OBJECTPATH,i);
            setProperty(bus,Object,"CommandSet",1);
            setProperty(bus,Object,"FloorDuty",static_cast<double>(floorDuty));
        }
    }
    else if (floorDuty == 0xff)
    {
        // Get Floor Duty Cycle
        memset(Object,0,sizeof(Object));
        snprintf(Object,sizeof(Object),"%s%d",FSC_OBJECTPATH,1);

        rc = getProperty(bus,Object,"FloorDuty",responseData,FSC_SERVICE,PID_INTERFACE);
        if(rc<0)
        {
            return ipmi::responseUnspecifiedError();
        }

        currentFloorDuty = static_cast<uint8_t>(responseData);

        rc = getProperty(bus,Object,"CommandSet",responseData,FSC_SERVICE,PID_INTERFACE);
        if(rc<0)
        {
            return ipmi::responseUnspecifiedError();
        }

        controlSource = static_cast<uint8_t>(responseData);
        if (controlSource == 1)
        {
            currentFloorDuty = currentFloorDuty + 0x80;
        }

        return ipmi::responseSuccess(currentFloorDuty);

    }
    else if (floorDuty == 0xfe)
    {
        //Set Floor Duty Cycle control by sensors
        for(size_t i = 1; i <= 5; i++)
        {
            memset(Object,0,sizeof(Object));
            snprintf(Object,sizeof(Object),"%s%d",FSC_OBJECTPATH,i);
            setProperty(bus,Object,"CommandSet",0);
        }
    }
    else
    {
        return ipmi::responseParmOutOfRange();
    }

    return ipmi::responseSuccess();
}
//===============================================================
/* Config EccLeaky Bucket Command
NetFun: 0x2E
Cmd : 0x1A
Request:
        Byte 1-3 : Tyan Manufactures ID (FD 19 00)
        Byte (4) : optional, set T1
        Byte (5) : optional, set T2

Response:
        Byte 1 : Completion Code
        Byte 2-4 : Tyan Manufactures ID
        Byte (5) : Return current T1 if request length = 3.
        Byte (6) : Return current T2 if request length = 3.
*/
ipmi::RspType<std::optional<uint8_t>, // T1
              std::optional<uint8_t>  // T2
              >
    ipmi_tyan_ConfigEccLeakyBucket(std::optional<uint8_t> T1, std::optional<uint8_t> T2)
{

    constexpr const char* leakyBucktPath =
        "/xyz/openbmc_project/sensors/leakyBucket/HOST_DIMM_ECC";
    constexpr const char* leakyBucktIntf =
        "xyz.openbmc_project.Sensor.Value";
    std::shared_ptr<sdbusplus::asio::connection> busp = getSdBus();
    uint8_t t1;
    uint8_t t2;
    ipmi::Value result;

    auto service = ipmi::getService(*busp, leakyBucktIntf, leakyBucktPath);

    //get t1,t2
    if(!T1)
    {
        try
        {
            result = ipmi::getDbusProperty(
                    *busp, service, leakyBucktPath, leakyBucktIntf, "T1");
            t1 = std::get<uint8_t>(result);

            result = ipmi::getDbusProperty(
                *busp, service, leakyBucktPath, leakyBucktIntf, "T2");
            t2 = std::get<uint8_t>(result);

        }
        catch (const std::exception& e)
        {
            return ipmi::responseUnspecifiedError();
        }
        return ipmi::responseSuccess(t1,t2);
    }

    //t1 found
    try
    {
       ipmi::setDbusProperty(
                *busp, service, leakyBucktPath, leakyBucktIntf, "T1", T1.value());
    }
    catch (const std::exception& e)
    {
        return ipmi::responseUnspecifiedError();
    }

    if(T2)
    {
        try
        {
            ipmi::setDbusProperty(
                    *busp, service, leakyBucktPath, leakyBucktIntf, "T2", T2.value());
        }
        catch (const std::exception& e)
        {
            return ipmi::responseUnspecifiedError();
        }
    }
    return ipmi::responseSuccess();

}

//===============================================================
/* get gpio status Command (for manufacturing)
NetFun: 0x2E
Cmd : 0x41
Request:
        Byte 1-3 : Tyan IANA ID (FD 19 00)
        Byte 4 : gpio number
Response:
        Byte 1 : Completion Code
        Byte 2-4 : Tyan IANA
        Byte 5 : gpio direction
        Byte 6 : gpio data
*/
ipmi::RspType<uint8_t,
              uint8_t>
    ipmi_tyan_getGpio(uint8_t gpioNo)
{
    auto chip = gpiod::chip("gpiochip0");
    auto line = chip.get_line(gpioNo);

    if (!line)
    {
        std::cerr << "Error requesting gpio\n";
        return ipmi::responseUnspecifiedError();
    }
    auto dir = line.direction();

    bool resp;
    try
    {
        line.request({"ipmid", gpiod::line_request::DIRECTION_INPUT,0});
        resp = line.get_value();
    }
    catch (std::system_error&)
    {
        std::cerr << "Error reading gpio: " << (unsigned)gpioNo << "\n";
        return ipmi::responseUnspecifiedError();
    }

    line.release();
    return ipmi::responseSuccess((uint8_t)dir, (uint8_t)resp);
}

//===============================================================
/* Power Node Manager Oem Get Reading
NetFun: 0x30
Cmd : 0xE2
Request:
        Byte 1 : [0:3] Domain ID / [4:7] Reading Type
        Byte 2 : Optional Parameter.
        Byte 3 : Reserved. Write as 00h.

Response:
        Byte 1 : Completion Code
        Byte 2 : [0:3] Domain ID / [4:7] Reading Type
        Byte 3-4 : Reading value 16-bit encoding 2s-complement signed integer.
*/

#define PIN_OBJECT "/xyz/openbmc_project/sensors/power/PSU0_Input_Power"
#define PIN_SERVICE "xyz.openbmc_project.PSUSensor"
#define PIN_INTERFACE "xyz.openbmc_project.Sensor.Value"

ipmi_ret_t ipmi_Pnm_GetReading(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                              ipmi_request_t request, ipmi_response_t response,
                              ipmi_data_len_t data_len, ipmi_context_t context)
{
    ipmi_ret_t ipmi_rc = IPMI_CC_OK;
    uint8_t domainID;
    uint8_t readingType;
    uint8_t highByte;
    uint8_t lowByte;
    int rc=0;
    double readingValue;
    uint8_t responseData[3]={0};

    auto bus = sdbusplus::bus::new_default();
    auto* requestData= reinterpret_cast<PnmGetReadingRequest*>(request);

    if((int)*data_len != 3)
    {
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    domainID = requestData->type & 0x0F;
    readingType = requestData->type >> 4;
    if (readingType == 0x00 || readingType == 0x06)
    {
        // get PSU PIN sensor reading value
        rc = getProperty(bus,PIN_OBJECT,"Value",readingValue,PIN_SERVICE,PIN_INTERFACE);
        if(rc<0)
        {
            return IPMI_CC_UNSPECIFIED_ERROR;
        }

        highByte = static_cast<uint16_t>(readingValue) >> 8;
        lowByte = static_cast<uint16_t>(readingValue) & 0x00FF;
        responseData[1]=lowByte;
        responseData[2]=highByte;
    }

    responseData[0]=requestData->type;
    memcpy(response, responseData, sizeof(responseData));

    return ipmi_rc;
}

//===============================================================
/* Set FRU Field Command
NetFun: 0x2E
Cmd : 0x0B
Request:
    Byte 1-3 : Tyan Manufactures ID (FD 19 00)
    Byte 4 :  FRU ID
    Byte 5 :  Item
        [7:4] : Area (1-Chassis Info , 2-Board Info , 3-Product Info)
        [3:0] : Field
    Byte 6-N : Data
Response:
    Byte 1 : Completion Code
    Byte 2-4 : Tyan Manufactures ID
*/
ipmi::RspType<> ipmi_setFruField(uint8_t fruId, uint4_t field, uint4_t area, std::vector<uint8_t> dataInfo)

{
    std::string s;
    size_t pos;
    char command[100];
    char cmd[100];
    char string[100];

    if(fruId != 0)
    {
        // Currently, only support FRU ID 0
        return ipmi::responseReqDataLenInvalid();
    }

    for(int i=0; i<dataInfo.size(); i++)
    {
        s += (char)dataInfo[i];
    }

    strcpy(string,s.c_str());
    memset(command,0,sizeof(command));

    switch ((uint8_t)area)
    {
        case 0:
            // flag
            snprintf(command,sizeof(command),"echo %d > /usr/sbin/fruFlag",dataInfo[0]);

            if(dataInfo[0] == 2)
            {
                memset(cmd,0,sizeof(cmd));
                snprintf(cmd,sizeof(cmd),"/usr/sbin/writeFRU.sh &");
                system(cmd);
            }
            break;
        case 1:
            // chassis information are
            snprintf(command,sizeof(command),"echo c %d %s >> /usr/sbin/fruWrite",(int)field,string);
            break;
        case 2:
            // board information area
            snprintf(command,sizeof(command),"echo b %d %s >> /usr/sbin/fruWrite",(int)field,string);
            break;
        case 3:
            // product information area
            snprintf(command,sizeof(command),"echo p %d %s >> /usr/sbin/fruWrite",(int)field,string);
            break;
        default:
            return ipmi::responseParmOutOfRange();
    }

    system(command);
    return ipmi::responseSuccess();
}

std::string getFruData()
{
    std::string str;
    char command[100];

    if(fs::exists("/usr/sbin/fruData"))
    {
        std::ifstream file("/usr/sbin/fruData", std::ios::in);

        auto data = std::vector<uint8_t>(std::istreambuf_iterator<char>(file),
                                     std::istreambuf_iterator<char>());
        if (file.fail())
        {
            std::cout << "read FRU Data fail" << std::endl;
        }
        file.close();

        for(int i=0; i<data.size(); i++)
        {
            str += (char)data[i];
        }
    }
    else
    {
        memset(command,0,sizeof(command));
        sprintf(command, "ipmitool fru print 0 > /usr/sbin/fruData &");
        system(command);
    }
    return str;
}

std::string areaData(std::string str, std::string str1, std::string str2)
{
    std::string string;
    std::string::size_type pos,pos1;
    std::string STR;

    pos = str.find(str1);
    if(pos != std::string::npos)
    {
        string = str.substr(pos);
        pos = string.find(":");
        pos1 = string.find(str2);
        if(pos1 != std::string::npos)
        {
            STR = string.substr(pos+2,(pos1-pos-3));
        }
        else
        {
            STR = string.substr (pos+2);
        }
    }
    return STR;
}

//===============================================================
/* Get FRU Field Command
NetFun: 0x2E
Cmd : 0x0C
Request:
    Byte 1-3 : Tyan Manufactures ID (FD 19 00)
    Byte 4 :  FRU ID
    Byte 5 :  Item
        [7:4] : Area (1-Chassis Info , 2-Board Info , 3-Product Info)
        [3:0] : Field
Response:
    Byte 1 : Completion Code
    Byte 2-4 : Tyan Manufactures ID
    Byte 5-N : Data
*/
ipmi::RspType<std::vector<uint8_t>> ipmi_getFruField(uint8_t fruId, uint4_t field, uint4_t area)
{

    int rc=0;
    char command[100],temp[50];
    char string[100];
    size_t pos;
    std::string str,str1,str2;
    uint8_t f;

    if(fruId != 0)
    {
        // Currently, only support FRU ID 0
        return ipmi::responseReqDataLenInvalid();
    }

    if(area > 0x3)
    {
        //fail Area
        return ipmi::responseInvalidFieldRequest();
    }
    else if(area == 0x0)
    {
        //flag
        if(fs::exists("/usr/sbin/fruFlag"))
        {
            memset(command,0,sizeof(command));
            memset(temp, 0, sizeof(temp));
            sprintf(command, "cat /usr/sbin/fruFlag");
            rc = execmd((char *)command, temp);
            if (rc != 0)
            {
                return ipmi::responseUnspecifiedError();
            }

            f = strtol(temp,NULL,10);
            std::vector<uint8_t> flag = {f};
            return ipmi::responseSuccess(flag);
        }
        else
        {
            return ipmi::responseInvalidFieldRequest();
        }
    }
    else
    {
        str=getFruData();

        switch ((uint8_t)area)
        {
            case 1:
                // chassis information are
                switch((uint8_t)field)
                {
                    case 0:
                        // chassis type
                        str2 = areaData(str,"Chassis Type","Chassis Part");
                        break;
                    case 1:
                        // chassis part number
                        str2 = areaData(str,"Chassis Part","Chassis Serial");
                        break;
                    case 2:
                        // chassis serial umber
                        str2 = areaData(str,"Chassis Serial","Board Mfg Date");
                        break;
                    default:
                        break;
                }
                break;
            case 2:
                // board information area
                pos = str.find("Board Mfg Date");
                str1 = str.substr(pos+14);
                switch((uint8_t)field)
                {
                    case 0:
                        // board manufacturer
                        str2 = areaData(str1,"Board Mfg","Board Product");
                        break;
                    case 1:
                        // board product name
                        str2 = areaData(str1,"Board Product","Board Serial");
                        break;
                    case 2:
                        // board serial number
                        str2 = areaData(str1,"Board Serial","Board Part");
                        break;
                    case 3:
                        // board part number
                        str2 = areaData(str1,"Board Part","Product Manufacturer");
                        break;
                    default:
                        break;
                }
                break;
            case 3:
                // product information area
                switch((uint8_t)field)
                {
                    case 0:
                        // product manufacturer
                        str2 = areaData(str,"Product Manufacturer","Product Name");
                        break;
                    case 1:
                        // product name
                        str2 = areaData(str,"Product Name","Product Part");
                        break;
                    case 2:
                        // product part / model number
                        str2 = areaData(str,"Product Part","Product Version");
                        break;
                    case 3:
                        // product version
                        str2 = areaData(str,"Product Version","Product Serial");
                        break;
                    case 4:
                        // product serial number
                        str2 = areaData(str,"Product Serial","Product Asset");
                        break;
                    default:
                        break;
                }
                break;
        }
    }

    if((char)str2[str2.length()-1] == 0xa)
    {
        str2.pop_back();
    }

    std::vector<uint8_t> DATA(str2.begin(), str2.end());
    return ipmi::responseSuccess(DATA);
}

//===============================================================

ipmi::RspType<std::vector<uint8_t>>
    ipmi_sendRawPeci(uint8_t clientAddr, uint8_t writeLength, uint8_t readLength,
                        std::vector<uint8_t> writeData)
{
    uint8_t PECI_BUFFER_SIZE = 0x80;

    if (readLength > PECI_BUFFER_SIZE)
    {
        log<level::ERR>("sendRawPeci command: Read length exceeds limit");
        return ipmi::responseParmOutOfRange();
    }
    std::vector<uint8_t> rawResp(readLength);
    if (peci_raw(clientAddr, readLength, writeData.data(), writeData.size(),
                             rawResp.data(), rawResp.size()) != PECI_CC_SUCCESS)
    {
        log<level::ERR>("sendRawPeci command: PECI command failed");
        return ipmi::responseResponseError();
    }

    return ipmi::responseSuccess(rawResp);

}

//===============================================================
/* Set specified service enable or disable
NetFun: 0x30
Cmd : 0x0D
Request:
    Byte 1 : Set service status
        [7-1] : reserved
        [0] :
            0h-Disable web service
            1h-Enable web service
Response:
    Byte 1 : Completion Code
*/
ipmi::RspType<> ipmi_SetService(uint8_t serviceSetting)
{
    constexpr auto service = "xyz.openbmc_project.Settings";
    constexpr auto path = "/xyz/openbmc_project/oem/ServiceStatus";
    constexpr auto serviceStatusInterface = "xyz.openbmc_project.OEM.ServiceStatus";
    constexpr auto webService = "WebService";

    auto bus = sdbusplus::bus::new_default();

    //Set web service status
    try
    {
        auto method = bus.new_method_call(service, path, PROPERTY_INTERFACE,"Set");
        method.append(serviceStatusInterface, webService, std::variant<int>(serviceSetting & 0x01));
        bus.call_noreply(method);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>("Error in RamdomDelayACRestorePowerON Set",entry("ERROR=%s", e.what()));
        return ipmi::responseParmOutOfRange();
    }

    return ipmi::responseSuccess();
}

//===============================================================
/* Get specified service enable or disable status
NetFun: 0x30
Cmd : 0x0E
Request:

Response:
    Byte 1 : Completion Code
    Byte 2 : Set service status
        [7-1] : reserved
        [0] :
            0h-Web service is disable
            1h-Web service is enable
*/
ipmi::RspType<uint8_t> ipmi_GetService()
{
    uint8_t serviceResponse = 0;

    constexpr auto service = "xyz.openbmc_project.Settings";
    constexpr auto path = "/xyz/openbmc_project/oem/ServiceStatus";
    constexpr auto serviceStatusInterface = "xyz.openbmc_project.OEM.ServiceStatus";
    constexpr auto webService = "WebService";

    auto bus = sdbusplus::bus::new_default();

    //Get web service status
    auto method = bus.new_method_call(service, path, PROPERTY_INTERFACE, "Get");
    method.append(serviceStatusInterface, webService);

    std::variant<bool> result;
    try
    {
        auto reply = bus.call(method);
        reply.read(result);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>("Error in PowerRestoreDelay Get",entry("ERROR=%s", e.what()));
        return ipmi::responseUnspecifiedError();
    }
    auto webServiceStatus = std::get<bool>(result);

    serviceResponse = (uint8_t)(webServiceStatus);

    return ipmi::responseSuccess(serviceResponse);
}

/*
* Print Test Command
*/

ipmi::RspType<uint8_t> printTest(uint8_t param)
{
    std::stringstream ss;
    ss << "Test commad is working. Param=0x" << std::hex << std::setfill('0') << std::setw(2) << +param;
    std::string ss_str = ss.str();
    phosphor::logging::log<phosphor::logging::level::INFO>(ss_str.c_str());
    return ipmi::responseSuccess(param);
}

/*
 * Set Front Panel (FP) LEDs
 * raw 0x2e 0x30 0x3d 0x2b 0x00 <Yellow> <Green> <Blue>
 * 0 - off
 * 1 - on
 * 2 - blink
 * 3 - Do nothing
 */
static constexpr const char* ledInterface = "xyz.openbmc_project.Led.Physical";

static std::vector<const char *> ledServices = { "xyz.openbmc_project.LED.Controller.status_amber",
                                                 "xyz.openbmc_project.LED.Controller.status_green",
                                                 "xyz.openbmc_project.LED.Controller.identify"};

static std::vector<const char *> ledObjPaths = { "/xyz/openbmc_project/led/physical/status_amber",
                                                 "/xyz/openbmc_project/led/physical/status_green",
                                                 "/xyz/openbmc_project/led/physical/identify"};

static std::vector<const char *> ledStates = { "xyz.openbmc_project.Led.Physical.Action.Off",
                                               "xyz.openbmc_project.Led.Physical.Action.On",
                                               "xyz.openbmc_project.Led.Physical.Action.Blink"};
/*
 * Set front panel led
 * raw 0x2e 0x30 0x3d 0x2b 0x00 [amber state] [green state] [identity state]
 * LED:
 *      0 - amber
 *      1 - green
 *      2 - identity
 * State:
 *      0 - off
 *      1 - on
 *      2 - blink
 *      3 - do nothing
*/
ipmi::RspType<std::vector<uint8_t>>ipmiSetFpLED(std::vector<uint8_t> param)
{
    if (param.size() != 3)
        return ipmi::responseParmOutOfRange();

    for(int i=0;i<3;i++)
    {
        if (param[i] > 3)
            return ipmi::responseParmOutOfRange();
    }

    std::shared_ptr<sdbusplus::asio::connection> dbus;
    try
    {
        dbus = getSdBus();
    }
    catch (std::system_error& e)
    {
        std::cerr << __FUNCTION__ << " Unable to get bus. error:" << e.what() << "\n";
        return ipmi::responseUnspecifiedError();
    }

    std::vector<unsigned char> response;
    response.clear();

    for(int i=0;i<3;i++)
    {
        try
        {
            if (param[i] != 3)
            {
                ipmi::setDbusProperty(*dbus, ledServices[i], ledObjPaths[i],
                            ledInterface, "State", ledStates[param[i]]);
            }
            response.push_back(param[i]);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            std::cerr << __FUNCTION__ << " " << ledServices[i] << " error:" << e.what() << "\n";
            response.push_back(0xff);
        }
    }
    return ipmi::responseSuccess(response);
}

/*
 * Control identity LED
 * raw 0x38 0x86 [state]
 * State:
 *       0 - off
 *       1 - on
 *       2 - blink
 */
ipmi::RspType<std::vector<uint8_t>> ipmiIdentityLedControl(uint8_t ledState)
{
    std::vector<uint8_t> param = {3, 3, ledState};
    return ipmiSetFpLED(param);
}

/*
 * Set JBOD LEDs and get HDD presence status
 * raw 0x2e 0x31 0x3d 0x2b 0x00 <sasHD> <hdd> [Red] [Green]
 * sasHD: Channel number <0..3>
 * hdd: HDD number <0..3>
 * sasHD: 0xff hdd: 0xff means "off all LEDs"
 * LED state:
 * 0 - off
 * 1 - on
 * 2 - Do nothing
 * Return:
 * 0..3 - miniSAS-HD number
 * 0..3 - HDD index
 * 0/1 - HDD not present/present
 * 0/1 - Red led off/on (0xff in case of error)
 * 0/1 - Green led off/on (0xff in case of error)
 */
ipmi::RspType<std::vector<uint8_t>>ipmiSetJBODLed(std::vector<uint8_t> param)
{
    uint16_t pca9555_word;

    /* Check input (channel number and HDD) */
    if (param.size() < 2)
        return ipmi::responseParmOutOfRange();

    /* Process "All off" command" */
    if (param[0] == 0xff && param[1] == 0xff)
    {
        std::cerr << __FUNCTION__ << " All Off cmd received\n";
        for(int bus=41; bus <=44; bus ++)
        {
            std::string i2cBus = "/dev/i2c-" + std::to_string(bus);
            unsigned long funcs = 0;
            int fd = open(i2cBus.c_str(), O_RDWR);
            if (fd < 0)
                continue;
            if (ioctl(fd, I2C_SLAVE_FORCE, 0x20) < 0 ||
                ioctl(fd, I2C_FUNCS, &funcs) < 0 ||
                !(funcs & I2C_FUNC_SMBUS_READ_WORD_DATA))
            {
                close(fd);
                continue;
            }
            /* Write LEDs to P0 (reg address is 2) */
            i2c_smbus_write_byte_data(fd, 0x02, 0xff);
            /* Write direction byte for P0 (reg address is 6) */
            i2c_smbus_write_byte_data(fd, 0x06, 0x00);
            close(fd);
        }

        /* Format and return response */
        std::vector<unsigned char> response;
        response.clear();
        response.push_back(static_cast<uint8_t>(0xff)); // sasHD
        response.push_back(static_cast<uint8_t>(0xff)); // HDD index
        response.push_back(static_cast<uint8_t>(0xff)); // Presence
        response.push_back(static_cast<uint8_t>(0x00)); // Red
        response.push_back(static_cast<uint8_t>(0x00)); // Green
        return ipmi::responseSuccess(response);
    }

    if (param[0] > 3 || param[1] > 3)
        return ipmi::responseParmOutOfRange();

    int hddIdx = param[1];

    /* First read PCA9555 word to save LED states and get presence */
    std::string i2cBus = "/dev/i2c-" + std::to_string(41 + param[0]);
    unsigned long funcs = 0;
    int fd = open(i2cBus.c_str(), O_RDWR);

    if (fd < 0)
    {
        std::cerr << __FUNCTION__  << " No bus: " << i2cBus << "\n";
        std::vector<unsigned char> response;
        response.clear();
        response.push_back(static_cast<uint8_t>(param[0])); // sasHD
        response.push_back(static_cast<uint8_t>(param[1])); // HDD index
        response.push_back(static_cast<uint8_t>(0xff)); // Presence
        response.push_back(static_cast<uint8_t>(0xff)); // Red
        response.push_back(static_cast<uint8_t>(0xff)); // Green
        return ipmi::responseSuccess(response);
    }
    if (ioctl(fd, I2C_SLAVE_FORCE, 0x20) < 0 ||
        ioctl(fd, I2C_FUNCS, &funcs) < 0 ||
        !(funcs & I2C_FUNC_SMBUS_READ_WORD_DATA))
    {
        std::cerr << __FUNCTION__  << " No PCA9555@0x20 device on bus " << i2cBus << "\n";
        close(fd);
        std::vector<unsigned char> response;
        response.clear();
        response.push_back(static_cast<uint8_t>(param[0])); // sasHD
        response.push_back(static_cast<uint8_t>(param[1])); // HDD index
        response.push_back(static_cast<uint8_t>(0xff)); // Presence
        response.push_back(static_cast<uint8_t>(0xff)); // Red
        response.push_back(static_cast<uint8_t>(0xff)); // Green
        return ipmi::responseSuccess(response);
    }

    /* Write direction byte for P1 (reg address is 7) */
    i2c_smbus_write_byte_data(fd, 0x07, 0xff);

    /* Read presence from input port register and led status from output port register */
    uint8_t p0_leds     = i2c_smbus_read_byte_data(fd, 0x02);
    std::cerr << __FUNCTION__ << " <<<< p0_leds:0x" << std::hex << std::setfill('0') << std::setw(2) << +p0_leds << "\n";
    uint8_t p1_presence = i2c_smbus_read_byte_data(fd, 0x01);
    std::cerr << __FUNCTION__ << " <<<< p1_presence:0x" << std::hex << std::setfill('0') << std::setw(2) << +p1_presence << "\n";

    /* Now check if we ordered to set LEDs */
    if (param.size() == 4)
    {
        /* HDD to PCA9555 pin
         * LED Ctrl / Detect
         * P00(G),P04(Y) / P10
         * P01(G),P05(Y) / P11
         * P02(G),P06(Y) / P12
         * P03(G),P07(Y) / P13
         */
        for(int color = 0; color < 2; color++) // 0 - red, 1 - green
        {
            uint8_t cmd = param[2+color];

            if (cmd > 2)
            {
                close(fd);
                return ipmi::responseParmOutOfRange();
            }

            if (cmd == 2)
                continue;

            uint8_t mask = 1 << hddIdx << ((1 - color)*4);
            //std::cerr << __FUNCTION__ << " idx:" << +hddIdx << " color:" << +color
            //          << " mask:0x" << std::hex << std::setfill('0') << std::setw(2) << +mask << "\n";
            p0_leds |= (mask * (1-cmd));
            p0_leds &= ~(mask*cmd);
        }
    }

    std::cerr << __FUNCTION__ << " >>>> p0_leds:0x" << std::hex << std::setfill('0') << std::setw(2) << +p0_leds << "\n";
    /* Do I2C write here */
    /* Write LEDs to P0 (reg address is 2) */
    i2c_smbus_write_byte_data(fd, 0x02, p0_leds);
    /* Write direction byte for P0 (reg address is 6) */
    i2c_smbus_write_byte_data(fd, 0x06, 0x00);

    /* Format response */
    std::vector<unsigned char> response;
    response.clear();
    response.push_back(param[0]); // Port number
    response.push_back(param[1]); // HDD number
    response.push_back(static_cast<uint8_t>(((~p1_presence) >> hddIdx) & 0x01)); // Presence
    response.push_back(static_cast<uint8_t>(((~p0_leds) >> (4 + hddIdx)) & 0x01)); // Red LED
    response.push_back(static_cast<uint8_t>(((~p0_leds) >> hddIdx) & 0x01)); // Green LED

    return ipmi::responseSuccess(response);
}

/*
* Get BIOS Lock/Unlock status
* raw 0x2e 0x21 0x3d 0x2b 0x00 0x24
* Response
* 3d 2b 00 <1 byte, 0: Unlock, 1: Lock>
*/
static bool biosLockOverride = false;

ipmi::RspType<std::vector<unsigned char>>ipmiOEMgetBiosLockStatus(uint8_t param){
    std::vector<unsigned char> response;
    response.clear();

    switch(param)
    {
        case SUB_CMD_FLIP_BIOS_LOCK_OVERRIDE:
        {
            // WARNING FIXME BUG TODO NOT_IN_PROD
            // biosLockOverride = !biosLockOverride;
            // WARNING FIXME BUG TODO NOT_IN_PROD
            if (biosLockOverride)
                std::cerr << "BIOS override flipped, now enabled\n";
            else
                std::cerr << "BIOS override flipped, now disabled\n";
            return ipmi::responseSuccess(response);
            break;
        }

        case SUB_CMD_GET_BIOS_LOCK_STATUS:
        {
            constexpr auto service = "xyz.openbmc_project.Chassis.Buttons";
            constexpr auto path = "/xyz/openbmc_project/chassis/buttons/lock";
            constexpr auto buttonStatusInterface = "xyz.openbmc_project.Chassis.Buttons";
            constexpr auto buttonStatus = "ButtonPressed";

            auto bus = sdbusplus::bus::new_default();

            auto method = bus.new_method_call(service, path, PROPERTY_INTERFACE, "Get");
            method.append(buttonStatusInterface, buttonStatus);

            std::variant<bool> result;

            //Delete this quick fix after checking
            /************************************/
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
            file.close();

            if(board == "my62" || board == "mzb2")
            {
                uint8_t value;

                try
                {
                    auto gpio_a2 = gpiod::find_line("FP_SW_1");
                    gpio_a2.request({"ipmid", gpiod::line_request::DIRECTION_INPUT,0});
                    uint8_t gpio_state = (uint8_t) gpio_a2.get_value();
                    gpio_a2.release();

                    std::string msg = "GPIO value = " + std::to_string(gpio_state) + ", BIOS is ";
                    if (gpio_state == 1)
                    {
                        msg += "unlocked";
                        value = 0x00;
                    }
                    else
                    {
                        msg += "locked";
                        value = 0x01;
                    }

                    phosphor::logging::log<phosphor::logging::level::INFO>(msg.c_str());

                    response.push_back(value);
                    return ipmi::responseSuccess(response);
                }
                catch (std::system_error&)
                {
                    phosphor::logging::log<phosphor::logging::level::ERR>("Error reading GPIO BIOS lock pin");
                    return ipmi::responseUnspecifiedError();
                }
            }
            /************************************/

            try
            {
                auto reply = bus.call(method);
                reply.read(result);
            }
            catch (const sdbusplus::exception::SdBusError& e)
            {
                std::cerr << __FUNCTION__ << " Error: " << e.what();
                return ipmi::responseUnspecifiedError();
            }

            if (biosLockOverride)
            {
                std::cerr << "BIOS lock override in action\n";
                uint8_t lock_state = 0;
                response.push_back(lock_state);
                return ipmi::responseSuccess(response);
            }

            //std::cerr << "BIOS is ";
            uint8_t lock_state = 0;
            if (std::get<bool>(result))
            {
                //std::cerr << "unlocked\n";
                lock_state = 0;
            }
            else
            {
                //std::cerr << "locked\n";
                lock_state = 1;
            }

            response.push_back(lock_state);
            return ipmi::responseSuccess(response);
            break;
        }
        default:
            std::cerr << __FUNCTION__ << "Unknown subcommand: 0x2e 0x21 0x3d 0x2b 0x00 0x"
                      << std::hex << std::setfill('0') << std::setw(2) << +param << "\n";
            return ipmi::responseParmOutOfRange();
            break;
    }

    return ipmi::responseParmOutOfRange();
}

void removeCharsFromString(std::string& str, char charsToRemove ) {
    str.erase( remove(str.begin(), str.end(), charsToRemove), str.end() );
}


/*
* BMC Information 1
* Get BMC basic Information
* raw 0x38 0x30 2
* Response
* <4 bytes, IP Address> <6 bytes, MAC Address> <6 bytes, System MAC Address 1> <6 bytes, System MAC Address 2>
* <6 bytes, System MAC Address 3> <6 bytes, System MAC Address 4>
*
* Get Tray Type
* raw 0x38 0x30 9
* Response <1 byte, 0: full-width node, 9: JBOD>
*/

ipmi::RspType<std::vector<unsigned char>>ipmigetNetworkData(uint8_t param)
{
    sdbusplus::bus::bus bus(ipmid_get_sd_bus_connection());

    const std::string ethdevice = "eth0";
    std::vector<unsigned char> response;
    response.clear();
    uint8_t cmdflagnode = 0x00;
    uint8_t cmdflagjbod = 0x09;

    switch (param)
    {
        case SUB_CMD_GET_BMC_BASIC_INFO:
            //IPv4
            try
            {
                auto ethIP = ethdevice + "/" + ipmi::network::IPV4_TYPE;
                std::string ipaddress;
                auto ipObjectInfo = ipmi::network::getIPObject(
                    bus, ipmi::network::IP_INTERFACE, ipmi::network::ROOT, ethIP);

                auto properties = ipmi::getAllDbusProperties(
                    bus, ipObjectInfo.second, ipObjectInfo.first,
                    ipmi::network::IP_INTERFACE);

                ipaddress = std::get<std::string>(properties["Address"]);
                phosphor::logging::log<phosphor::logging::level::INFO>(ipaddress.c_str());

                std::stringstream ss(ipaddress);
                std::string hex;
                while ( getline(ss, hex, '.') )
                {
                    std::stringstream hss(hex);
                    unsigned int temp;
                    hss >> temp;
                    response.push_back(temp);
                }
            }
            catch (std::system_error&)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>("Error reading IPv4 Address");
                return ipmi::responseUnspecifiedError();
            }

            //IPMI MAC
            try
            {
                std::string macAddress;
                auto macObjectInfo = ipmi::getDbusObject(bus, ipmi::network::MAC_INTERFACE, ipmi::network::ROOT, ethdevice);

                auto variant = ipmi::getDbusProperty(
                    bus, macObjectInfo.second, macObjectInfo.first,
                    ipmi::network::MAC_INTERFACE, "MACAddress");

                macAddress = std::get<std::string>(variant);
                phosphor::logging::log<phosphor::logging::level::INFO>(macAddress.c_str());

                std::stringstream mss(macAddress);
                std::string hex;
                while ( getline(mss, hex, ':') )
                {
                    std::stringstream hss(hex);
                    unsigned int temp;
                    hss >> std::hex >> temp;
                    response.push_back(temp);
                }
            }
            catch (std::system_error&)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>("Error reading IPMI MAC Address");
                return ipmi::responseUnspecifiedError();
            }

            //NIC MAC
            try
            {
                std::string nicMacAddress;
                for (int i=0; i<=4; i++)
                {
                    try
                    {
                        auto nicObjectInfo = ipmi::getDbusObject(bus, ipmi::network::ETH_INTERFACE, ipmi::network::MDR_ROOT, "/nic" + std::to_string(i));

                        auto variant = ipmi::getDbusProperty(
                            bus, nicObjectInfo.second, nicObjectInfo.first,
                            ipmi::network::ETH_INTERFACE, "nicMACAddress");

                        nicMacAddress = std::get<std::string>(variant);
                    }
                    catch(...)
                    {
                        phosphor::logging::log<phosphor::logging::level::ERR>("Error reading NIC MAC Address, breaking search loop");
                        break;
                    }
                    phosphor::logging::log<phosphor::logging::level::INFO>(nicMacAddress.c_str());

                    std::stringstream mss(nicMacAddress);
                    std::string hex;
                    while ( getline(mss, hex, ':') )
                    {
                        std::stringstream hss(hex);
                        unsigned int temp;
                        hss >> std::hex >> temp;
                        response.push_back(temp);
                    }
                }
            }
            catch (std::system_error&)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>("Error reading NIC MAC Address");
                //return ipmi::responseUnspecifiedError();
            }

            int cnt;
            cnt = 40 - response.size();
            for (int i=0; i<cnt; i++)
            {
                std::stringstream hss("ff");
                unsigned int temp;
                hss >> std::hex >> temp;
                response.push_back(temp);
            }

            return ipmi::responseSuccess(response);
            break;

       case SUB_CMD_GET_TRAY_TYPE:
            try
            {
                std::string traytype = "Node";

                response.push_back(cmdflagnode);
                return ipmi::responseSuccess(response);
                break;
            }
            catch (std::system_error&)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>("Error reading product name");
                return ipmi::responseUnspecifiedError();
            }

            return ipmi::responseSuccess(response);
            break;

        default:

            std::stringstream ss;
            ss << "Unknown subcommand: 0x38 0x30 0x3d 0x2b 0x00 0x" << std::hex << std::setfill('0') << std::setw(2) << +param;
            std::string ss_str = ss.str();
            phosphor::logging::log<phosphor::logging::level::ERR>(ss_str.c_str());

            return ipmi::responseParmOutOfRange();
            break;
    }
    return ipmi::responseParmOutOfRange();
}

/*
* Get Share Memory Address
* raw 0x2e 0x21 0x0a 0x3c 0x00 0x0a
* Response 0x0a 0x3c 0x00 <4 bytes, Address. LSB first>
*
* Get IPv6 Address
* raw 0x2e 0x21 0x0a 0x3c 0 4 2
* Response
* 0a 3c 0 <N bytes, IPv6 Address>
*
* Get IPv6 Link Local
* raw 0x2e 0x21 0x0a 0x3c 0 4 4
* Response
* 0a 3c 0 <N bytes, Link Local>
*
* BIOS Version
* Get BIOS Version
* raw 0x2e 0x21 0x0a 0x3c 0 19 1 0
* Response:
* 0a 3c 0 <N bytes, BIOS Version>
*/

ipmi::RspType<std::vector<uint8_t>>ipmiOEMgetBunchOfData(std::vector<uint8_t> param){

    sdbusplus::bus::bus bus(ipmid_get_sd_bus_connection());
    const std::string ethdevice = "eth0";
    uint8_t cmdflagfirst = 0x01;
    uint8_t cmdflagsecond = 0x00;
    std::vector<uint8_t> response;
    response.clear();

    switch(param[0])
    {
        case SUB_CMD_GET_SHARED_MEMORY_ADDRESS:
            try
            {
                std::stringstream ss;
                ss << "VGA shared memory address is " << std::hex << std::setfill('0') << std::setw(2) << +mdr2SMBaseAddress;
                std::string ss_str = ss.str();
                phosphor::logging::log<phosphor::logging::level::INFO>(ss_str.c_str());
                std::vector<uint8_t> raw_data( (uint8_t*)&mdr2SMBaseAddress
                                   , (uint8_t*)&(mdr2SMBaseAddress) + sizeof(uint32_t));
                return ipmi::responseSuccess(raw_data);
            }
            catch (std::system_error&)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>("Error to get Memory Address");
                return ipmi::responseUnspecifiedError();
            }

        case SUB_CMD_GET_BMC_IP:

            switch(param[1])
            {
                case SUB_CMD_GET_BMC_IPv6:
                    try
                    {
                        auto ethIP = ethdevice + "/" + ipmi::network::IPV6_TYPE;
                        std::string ipaddress;
                        auto ipObjectInfo = ipmi::network::getIPObject(
                           bus, ipmi::network::IP_INTERFACE, ipmi::network::ROOT, ethIP);

                        auto properties = ipmi::getAllDbusProperties(
                           bus, ipObjectInfo.second, ipObjectInfo.first,ipmi::network::IP_INTERFACE);

                        ipaddress = std::get<std::string>(properties["Address"]);

                        phosphor::logging::log<phosphor::logging::level::INFO>(ipaddress.c_str());

                        response.assign(ipaddress.begin(), ipaddress.end());

                        return ipmi::responseSuccess(response);
                        break;
                    }
                    catch (std::system_error&)
                    {
                        phosphor::logging::log<phosphor::logging::level::ERR>("Error getting IPv6 Address");
                        return ipmi::responseUnspecifiedError();
                    }

                case SUB_CMD_GET_BMC_IPv6_LOCAL:
                    try
                    {
                        auto ethIP = ethdevice + "/" + ipmi::network::IPV6_TYPE;
                        std::string ipaddress;
                        auto ipObjectInfo = ipmi::network::getIPLocalObject(
                           bus, ipmi::network::IP_INTERFACE, ipmi::network::ROOT, ethIP);

                        auto properties = ipmi::getAllDbusProperties(
                           bus, ipObjectInfo.second, ipObjectInfo.first,ipmi::network::IP_INTERFACE);
                        ipaddress = std::get<std::string>(properties["Address"]);

                        phosphor::logging::log<phosphor::logging::level::INFO>(ipaddress.c_str());
                        response.assign(ipaddress.begin(), ipaddress.end());

                        return ipmi::responseSuccess(response);
                        break;
                    }
                    catch (std::system_error&)
                    {
                        phosphor::logging::log<phosphor::logging::level::ERR>("Error getting IPv6 Local Address");
                        return ipmi::responseUnspecifiedError();
                    }
                default:
                    std::stringstream ss;
                    ss << "Unknown subcommand: " << std::hex << std::setfill('0') << std::setw(2) << +param[1];
                    std::string ss_str = ss.str();
                    phosphor::logging::log<phosphor::logging::level::ERR>(ss_str.c_str());

                    return ipmi::responseParmOutOfRange();
                    break;
            }

       case SUB_CMD_GET_BIOS_VERSION:

             try
             {
                 if(param[1] == cmdflagfirst && param[2] == cmdflagsecond){

                     std::string biosVersion;

                     auto biosObjectInfo = ipmi::getDbusObject(bus, ipmi::network::BIOS_INTERFACE, ipmi::network::MDR_ROOT, "/bios");

                     auto variant = ipmi::getDbusProperty(
                         bus, biosObjectInfo.second, biosObjectInfo.first,
                         ipmi::network::BIOS_INTERFACE, "Version");

                     biosVersion = std::get<std::string>(variant);
                     phosphor::logging::log<phosphor::logging::level::INFO>(biosVersion.c_str());
                     response.assign(biosVersion.begin(), biosVersion.end());
                     return ipmi::responseSuccess(response);
                     break;
                 }
            }
            catch (std::system_error&)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>("Error to read BIOS version");
                return ipmi::responseUnspecifiedError();
            }

       default:
            std::stringstream ss;
            ss << "Unknown subcommand: " << std::hex << std::setfill('0') << std::setw(2) << +param[0];
            std::string ss_str = ss.str();
            phosphor::logging::log<phosphor::logging::level::ERR>(ss_str.c_str());

            return ipmi::responseParmOutOfRange();
            break;
    }
    return ipmi::responseParmOutOfRange();
}

/*
* SMBIOS Data Ready on Share Memory
* Request raw 0x2e 0x20 0x0a 0x3c 0x00 0x0b <2 bytes, Request Length> <2 bytes, Reserved>
* Response 0x0a 0x3c 0x00
*/

ipmi::RspType<std::vector<uint8_t>>ipmiOEMreadyOnShareMemory(std::vector<uint8_t> param){
    std::stringstream ss;
    std::string ss_str;

    char command[100];
    std::ofstream("/var/lib/smbios/smbios2");

    std::stringstream cmd;
    std::string cmd_str;

    static constexpr const char* setBiosActiveObjPath = "/xyz/openbmc_project/software/bios_active";
    static constexpr const char* setBiosVersionIntf = "xyz.openbmc_project.Software.Version";
    static constexpr const char* setBiosVersionProp = "Version";
    static constexpr const char* biosActiveObjPath = "/xyz/openbmc_project/inventory/system/chassis/motherboard/bios";
    static constexpr const char* biosVersionIntf = "xyz.openbmc_project.Inventory.Decorator.Revision";
    static constexpr const char* biosVersionProp = "Version";
    std::string biosVersion;
    sdbusplus::bus::bus bus(ipmid_get_sd_bus_connection());

    static constexpr const char* setChassisActiveObjPath = "/xyz/openbmc_project/state/os";
    static constexpr const char* setChassisIntf = "xyz.openbmc_project.State.Host";
    static constexpr const char* setChassisProp = "OperatingSystemState";
    static constexpr const char* ChassisIntf = "xyz.openbmc_project.State.OperatingSystem.Status";

    switch(param[0])
    {
        case SUB_CMD_SMBIOS_DATA_READY_ON_SHARED_MEMORY:

            //Set BUS property
            try
            {
                std::cerr << "Setting Power State Property Run" << "\n";
                std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
                //std::string service = ipmi::getService(*dbus, setChassisIntf, setChassisActiveObjPath);
                //ipmi::setDbusProperty(*dbus, service, ChassisIntf, setChassisActiveObjPath, "OperatingSystemState", "Standby");
                ipmi::setDbusProperty(*dbus, setChassisIntf, setChassisActiveObjPath, ChassisIntf, "OperatingSystemState", "Standby");
                std::cerr << "Setting Power State Property" << "\n";
            }
            catch (const std::exception& e)
            {
                std::cerr << "Error, to set property for chassis" << e.what() << "\n";
            }

            ss << "Data ready in shared memory: 0x";

            for(auto b : param) {
                ss << std::hex << std::setfill('0') << std::setw(2) << +b;
            }

            ss_str = ss.str();
            phosphor::logging::log<phosphor::logging::level::INFO>(ss_str.c_str());
            cmd << "mmio_dump 0x";
            cmd << std::hex << +mdr2SMBaseAddress;
            /* FIXME TODO
             * As reply from Asus and Gigabyte differs, let's just read 65535 bytes of data. */
            param[3] = param[4] = 0xff;
            cmd << " 0x";
            cmd << std::hex << std::setfill('0') << std::setw(2) << +param[4];
            cmd << std::hex << std::setfill('0') << std::setw(2) << +param[3];
            cmd << " -b > /var/lib/smbios/smbios2";
            std::cerr << cmd.str() << "\n";
            cmd_str = cmd.str();

            phosphor::logging::log<phosphor::logging::level::INFO>(cmd_str.c_str());

            try
            {
                memset(command,0,sizeof(command));
                std::copy(cmd_str.begin(), cmd_str.end(), command);
                system(command);

                std::ofstream smbios_size("/var/lib/smbios/smbios2.size");
                smbios_size << std::hex << std::setfill('0') << std::setw(2) << +param[4];
                smbios_size << std::hex << std::setfill('0') << std::setw(2) << +param[3];
                smbios_size << "\n";
                smbios_size.close();

                /*
                ****
                * Call service restart if any hardware operations was on running machine
                ****
                */

                std::stringstream cmdRestartSmbiosApp;
                std::string cmd_restart;
                cmdRestartSmbiosApp << "systemctl restart --no-block smbios-mdrv2.service";
                std::cerr << cmdRestartSmbiosApp.str() << "\n";
                cmd_restart = cmdRestartSmbiosApp.str();

                memset(command,0,sizeof(command));
                std::copy(cmd_restart.begin(), cmd_restart.end(), command);
                system(command);
            }
            catch (std::system_error&)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>("Error, not ready on shared memory");
                return ipmi::responseUnspecifiedError();
            }

            //Get BIOS version from MDRv2
            try
            {
                auto biosObjectInfo = ipmi::getDbusObject(bus, ipmi::network::BIOS_INTERFACE, ipmi::network::MDR_ROOT, "/bios");

                auto variant = ipmi::getDbusProperty(
                         bus, biosObjectInfo.second, biosObjectInfo.first,
                         ipmi::network::BIOS_INTERFACE, "Version");

                biosVersion = std::get<std::string>(variant);
                phosphor::logging::log<phosphor::logging::level::INFO>(biosVersion.c_str());
            }
            catch (std::system_error&)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>("Error, to get BIOS Version Property");
            }

            //Set BIOS version to software manager property
            try
            {
                std::string result = "No BIOS Version";
                std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
                std::string service = ipmi::getService(*dbus, setBiosVersionIntf, setBiosActiveObjPath);
                ipmi::setDbusProperty(*dbus, service, setBiosActiveObjPath, setBiosVersionIntf, setBiosVersionProp, biosVersion);
            }
            catch (std::system_error&)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>("Error, to set BIOS Version Property");
            }

            return ipmi::responseSuccess();

        default:
            ss << "Unknown subcommand : 0x";
            for(auto b : param) {
                ss << std::hex << std::setfill('0') << std::setw(2) << +b;
            }
            ss_str = ss.str();
            phosphor::logging::log<phosphor::logging::level::ERR>(ss_str.c_str());

            return ipmi::responseParmOutOfRange();
            break;
    }
    return ipmi::responseParmOutOfRange();
}

/*
* Internal flag BIOS request platform name
* Request raw 0x2e 0x11 0x0A 0x3C 0x00 0x16
* Response 0x0a 0x3c 0x00 0x01
*/

/*
* Request
* Data1 = 0x0A
* Data2 = 0x3C
* Data3 = 0x0
* Data4 = 0x01
* Response
* Resp[1] = Completion Code
* Resp[2-4] = 0x0A, 0x3C, 0x0
* Resp[5-8] = Rand code (Please see below)
* 0x36 0xD2 0x8A 0x29 : <MY62>
* 0xFE 0xA6 0x17 0x33 : <MZB2>

* Resp[9-10] = Product ID (Please see below)
*  0x64 0x01 : <MY62>
* 0x39 0x10 : <MZB2>

* Resp[11-12] = 0x00 0x01
* Resp[13-14] = 0x02 0x0b
*/

ipmi::RspType<std::vector<uint8_t>>ipmiOEMaskInternalFlag(uint8_t param){
    uint8_t intflag = 0x01;
    std::string board = "board";
    std::string line;
    std::vector<uint8_t> response;
    std::vector<uint8_t> my62 = {0x36, 0xd2, 0x8a, 0x29};
    std::vector<uint8_t> mzb2 = {0xfe, 0xa6, 0x17, 0x33};
    std::vector<uint8_t> my62_prodID = {0x64, 0x01};
    std::vector<uint8_t> mzb2_prodID = {0x39, 0x10};
    std::vector<uint8_t> huemoe_first = {0x00, 0x01};
    std::vector<uint8_t> huemoe_second = {0x02, 0x0b};

    switch(param)
    {
        case SUB_CMD_INT_FLAG:
            try
            {
                response.push_back(intflag);
                phosphor::logging::log<phosphor::logging::level::INFO>("Send internal Flag to BIOS");
                return ipmi::responseSuccess(response);
            }
            catch (std::system_error&)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>("Error to send internal flag 0x01");
                return ipmi::responseUnspecifiedError();
            }

        case SUB_CMD_PLATFORM_FLAG:
            try
            {
                std::ifstream file;
                file.open("/usr/share/openrack/machine", std::ios::in);
                if (!file.is_open())
                {
                    throw std::runtime_error("Unable to open /usr/share/openrack/machine file");
                }

                while (getline(file, line)) {
                    if (line.find("model=") != std::string::npos) {
                        line.erase(0, 6);
                        std::stringstream iss(line);
                        iss >> board;
                        break;
                    }

                }

                if(board == "my81" || board == "mz81")
                {
                    response.push_back(intflag);
                    phosphor::logging::log<phosphor::logging::level::INFO>("Internal Flag received");
                    return ipmi::responseSuccess(response);
                }

                if(board == "my62")
                {
                    for (auto i : my62)
                    {
                        response.push_back(i);
                    }

                    for (auto i : my62_prodID)
                    {
                        response.push_back(i);
                    }

                    for (auto i : huemoe_first)
                    {
                        response.push_back(i);
                    }

                    for (auto i : huemoe_second){
                        response.push_back(i);
                    }

                    return ipmi::responseSuccess(response);
                }

                if(board == "mzb2")
                {
                    for (auto i : mzb2)
                    {
                        response.push_back(i);
                    }

                    for (auto i : mzb2_prodID)
                    {
                        response.push_back(i);
                    }

                    for (auto i : huemoe_first)
                    {
                        response.push_back(i);
                    }

                    for (auto i : huemoe_second){
                        response.push_back(i);
                    }

                    return ipmi::responseSuccess(response);
                }
            }
            catch (std::system_error&)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>("Error to send platform info to BIOS");
                return ipmi::responseUnspecifiedError();
            }
        default:

            std::stringstream ss;
            ss << "Unknown subcommand : " << std::hex << std::setfill('0') << std::setw(2) << +param;
            std::string ss_str = ss.str();
            phosphor::logging::log<phosphor::logging::level::ERR>(ss_str.c_str());

            return ipmi::responseParmOutOfRange();
            break;
    }
    return ipmi::responseParmOutOfRange();
}

/*
 * Gigabyte internal command, just sending back 0x04
 */
ipmi::RspType<uint8_t>ipmiOEMcmdUnkOne(){
    uint8_t intflag = 0x04;

    try
    {
        phosphor::logging::log<phosphor::logging::level::INFO>("Sending Internal Flag to BIOS 0x04");
        return ipmi::responseSuccess(intflag);
    }
    catch (std::system_error&)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>("Error to send Internal Flag 0x04");
        return ipmi::responseUnspecifiedError();
    }
    return ipmi::responseParmOutOfRange();
}

/*
 *
 */

ipmi::RspType<uint8_t> ipmiOEMcmdEnableDesableSensorsPolling(uint8_t cmd1, uint8_t cmd2)
{
    char command[600];
    std::string cmd_str;
    uint8_t response = 0x00;

    switch(cmd1)
    {
        case SUB_SENSORS_POLLING_CMD:
            try
            {
                std::stringstream cmd;

                if (cmd2 == 0x00)
                {
                    cmd << "systemctl stop xyz.openbmc_project.adcsensor.service";
                    cmd << " xyz.openbmc_project.healthsensor.service";
                    cmd << " xyz.openbmc_project.healthsensor.service";
                    cmd << " xyz.openbmc_project.mcutempsensor.service";
                    cmd << " xyz.openbmc_project.nvmelegacysensor.service";
                    cmd << " xyz.openbmc_project.sysfssensor.service";
                    cmd << " xyz.openbmc_project.gpusensor.service";
                    cmd << " xyz.openbmc_project.fansensor.service";
                    cmd << " xyz.openbmc_project.i2csensor.service";
                    cmd << " xyz.openbmc_project.EntityManager.service";
                    cmd << " xyz.openbmc_project.FruDevice.service";
                    cmd << " ipmb.service";
                    std::cerr << cmd.str() << "\n";
                    cmd_str = cmd.str();
                }
                else if (cmd2 == 0x01)
                {
                    cmd << "systemctl start --no-block xyz.openbmc_project.EntityManager.service";
                    cmd << " xyz.openbmc_project.adcsensor.service";
                    cmd << " xyz.openbmc_project.healthsensor.service";
                    cmd << " xyz.openbmc_project.healthsensor.service";
                    cmd << " xyz.openbmc_project.mcutempsensor.service";
                    cmd << " xyz.openbmc_project.nvmelegacysensor.service";
                    cmd << " xyz.openbmc_project.sysfssensor.service";
                    cmd << " xyz.openbmc_project.gpusensor.service";
                    cmd << " xyz.openbmc_project.fansensor.service";
                    cmd << " xyz.openbmc_project.i2csensor.service";
                    cmd << " xyz.openbmc_project.FruDevice.service";
                    cmd << " ipmb.service";
                    std::cerr << cmd.str() << "\n";
                    cmd_str = cmd.str();
                }
                memset(command,0,sizeof(command));
                std::copy(cmd_str.begin(), cmd_str.end(), command);
                system(command);
                response = 0x01;
            }
            catch (std::system_error&)
            {
                std::cerr << "Error, to control EntityManager.service" << "\n";
                return ipmi::responseUnspecifiedError();
            }
            return ipmi::responseSuccess(response);
            break;

        case SUB_GPU_POLLING_CMD:
            try
            {
                std::stringstream cmd;

                if (cmd2 == 0x00)
                {
                    cmd << "systemctl stop --no-block xyz.openbmc_project.gpusensor.service";
                    std::cerr << cmd.str() << "\n";
                    cmd_str = cmd.str();
                }
                else if (cmd2 == 0x01)
                {
                    cmd << "systemctl start --no-block xyz.openbmc_project.gpusensor.service";
                    std::cerr << cmd.str() << "\n";
                    cmd_str = cmd.str();
                }
                memset(command,0,sizeof(command));
                std::copy(cmd_str.begin(), cmd_str.end(), command);
                system(command);
                response = 0x01;
            }
            catch (std::system_error&)
            {
                std::cerr << "Error, to control gpusensor.service" << "\n";
                return ipmi::responseUnspecifiedError();
            }
            return ipmi::responseSuccess(response);
            break;

        default:
            std::stringstream ss;
            ss << "Unknown subcommand : " << std::hex << std::setfill('0') << std::setw(2) << +cmd1;
            std::string ss_str = ss.str();
            phosphor::logging::log<phosphor::logging::level::ERR>(ss_str.c_str());

            return ipmi::responseParmOutOfRange();
            break;
    }
    return ipmi::responseUnspecifiedError();
}

/*
 * @brief Ipmb utils
 */
using IpmbDbusRspType =
    std::tuple<int, uint8_t, uint8_t, uint8_t, uint8_t, std::vector<uint8_t>>;

int ipmbSendRequest(sdbusplus::asio::connection &conn,
                    IpmbDbusRspType &ipmbResponse,
                    const std::vector<uint8_t> &dataToSend, uint8_t netFn,
                    uint8_t lun, uint8_t cmd)
{

    constexpr const char *ipmbBus = "xyz.openbmc_project.Ipmi.Channel.Ipmb";
    constexpr const char *ipmbObj = "/xyz/openbmc_project/Ipmi/Channel/Ipmb";
    constexpr const char *ipmbIntf = "org.openbmc.Ipmb";

    constexpr uint8_t ipmbMeChannelNum = 1;

    try
    {
        auto mesg =
            conn.new_method_call(ipmbBus, ipmbObj, ipmbIntf, "sendRequest");
        mesg.append(ipmbMeChannelNum, netFn, lun, cmd, dataToSend);
        auto ret = conn.call(mesg);
        ret.read(ipmbResponse);
        return 0;
    }
    catch (sdbusplus::exception::SdBusError &e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "ipmbSendRequest:, dbus call exception");
        return -1;
    }
}

/*
 * Right now we can't use -t -b keys in order to get ME fw version
 * Hook to get ME firmware
 * raw 0x38 0x74
 * 50 01 04 43 02 21 57 01 00 0f 0b 04 26 30 01
 *
 */
ipmi::RspType<std::vector<uint8_t>> ipmiOEMGetMeFwVersion()
{
    constexpr uint8_t ipmiGetDevIdNetFn = 0x6;
    constexpr uint8_t ipmiGetDevIdLun = 0;
    constexpr uint8_t ipmiGetDevIdCmd = 0x1;

    std::vector<uint8_t> commandData;

    std::shared_ptr<sdbusplus::asio::connection> conn = getSdBus();

    phosphor::logging::log<phosphor::logging::level::INFO>("Send request to ME");

    std::vector<uint8_t> dataToSend;

    std::string board;
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

    if(board == "mzb2" || board == "mz81")
    {
        std::vector<uint8_t> response;
        response.push_back(0x00);
        phosphor::logging::log<phosphor::logging::level::INFO>("AMD node does not have ME, are you sure about your request?");
        return ipmi::responseSuccess(response);
    }

    IpmbDbusRspType ipmbResponse;
    int sendStatus = ipmbSendRequest(*conn, ipmbResponse, dataToSend, ipmiGetDevIdNetFn, ipmiGetDevIdLun, ipmiGetDevIdCmd);

    if (sendStatus)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>("Invalid ME Version");
        return ipmi::responseUnspecifiedError();
    }

    const auto& [status, netfn, lun, command, cc, dataReceived] = ipmbResponse;

    if (status)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
                "ipmiOEMGetMeFwVersion: ipmb non-zero response status ");
        return ipmi::responseUnspecifiedError();
    }
    if (cc)
    {
        phosphor::logging::log<phosphor::logging::level::WARNING>(
                "ipmiOEMGetMeFwVersion: non-zero completion code ");
        return ipmi::responseUnspecifiedError();
    }

    auto getDevIdResp =
            reinterpret_cast<const ipmiGetDeviceIdResp *>(dataReceived.data());

    auto major = std::to_string(getDevIdResp->fwMajorMinor.fwMajorRev);
    auto minor = std::to_string(getDevIdResp->fwMajorMinor.fwMinorRev);
    auto hotfix = std::to_string(getDevIdResp->fwMajorMinor.fwHotfixRev);
    auto build = std::to_string(getDevIdResp->fwVerAux.a) +
                     std::to_string(getDevIdResp->fwVerAux.b) +
                     std::to_string(getDevIdResp->fwVerAux.c);
    auto patch = std::to_string(getDevIdResp->fwVerAux.patch);

    std::stringstream ss;
    ss << "ME version major : " << major << "\n";
    ss << "ME version minor : " << minor << "\n";
    ss << "ME version hotfix : " << hotfix << "\n";
    ss << "ME version build : " << build << "\n";
    ss << "ME version patch : " << patch << "\n";

    for (auto d : dataReceived)
    {
        ss << std::hex << std::setfill('0') << std::setw(2) << +d << ":";
    }

    std::string ss_str = ss.str();
    phosphor::logging::log<phosphor::logging::level::INFO>(ss_str.c_str());

    std::vector<uint8_t> response(dataReceived.begin(), dataReceived.end());

    return ipmi::responseSuccess(response);
}

/*
 * Right now we can't use -t -b keys in order to get ME status
 * Hook to get ME status
 * raw 0x38 0x75
 */

ipmi::RspType<std::vector<uint8_t>> ipmiOEMGetMeStatus()
{
    constexpr uint8_t ipmiGetDevIdNetFn = 0x6;
    constexpr uint8_t ipmiGetDevIdLun = 0;
    constexpr uint8_t ipmiGetDevIdCmd = 0x4;

    std::vector<uint8_t> commandData;

    std::shared_ptr<sdbusplus::asio::connection> conn = getSdBus();

    phosphor::logging::log<phosphor::logging::level::INFO>("Send request to ME");

    std::vector<uint8_t> dataToSend;

    std::string board;
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

    if(board == "mzb2" || board == "mz81")
    {
        std::vector<uint8_t> response;
        response.push_back(0x00);
        phosphor::logging::log<phosphor::logging::level::INFO>("AMD node does not have ME, are you sure about your request?");
        return ipmi::responseSuccess(response);
    }

    IpmbDbusRspType ipmbResponse;
    int sendStatus = ipmbSendRequest(*conn, ipmbResponse, dataToSend, ipmiGetDevIdNetFn, ipmiGetDevIdLun, ipmiGetDevIdCmd);

    if (sendStatus)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>("Invalid ME Status");
        return ipmi::responseUnspecifiedError();
    }

    const auto& [status, netfn, lun, command, cc, dataReceived] = ipmbResponse;

    if (status)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
                "ipmiOEMGetMeStatus: ipmb non-zero response status ");
        return ipmi::responseUnspecifiedError();
    }
    if (cc)
    {
        phosphor::logging::log<phosphor::logging::level::WARNING>(
                "ipmiOEMGetMeStatus: non-zero completion code ");
        return ipmi::responseUnspecifiedError();
    }

    auto getDevIdResp =
            reinterpret_cast<const ipmiGetDeviceIdResp *>(dataReceived.data());

    std::stringstream ss;

    for (auto d : dataReceived)
    {
        ss << std::hex << std::setfill('0') << std::setw(2) << +d << ":";
    }

    std::string ss_str = ss.str();
    phosphor::logging::log<phosphor::logging::level::INFO>(ss_str.c_str());

    std::vector<uint8_t> response(dataReceived.begin(), dataReceived.end());

    return ipmi::responseSuccess(response);
}

ipmi::RspType<std::vector<uint8_t>> ipmiGetPostCode(uint8_t param)
{
    constexpr auto postCodeInterface = "xyz.openbmc_project.State.Boot.PostCode";
    constexpr auto postCodeObjPath   = "/xyz/openbmc_project/State/Boot/PostCode0";
    constexpr auto postCodeService   = "xyz.openbmc_project.State.Boot.PostCode0";

    uint16_t bootCycleIndex = 0;
    std::vector<postcode_t> tmpBuffer;
    int tmpBufferIndex = 0;
    std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
    std::vector<uint8_t> dataInfo;
    dataInfo.clear();
    tmpBuffer.clear();

    switch(param)
    {
        case SEB_CMD_GET_BIOS_POST:
            try
            {
                /* Get CurrentBootCycleIndex property */
                auto value = ipmi::getDbusProperty(*dbus, postCodeService, postCodeObjPath,
                               postCodeInterface, "CurrentBootCycleCount");

                /*Index indicates which boot cycle of post codes is requested.
                  1 is for the most recent boot cycle. CurrentBootCycleCount is for the
                  oldest boot cycle.*/
                bootCycleIndex = 1; //std::get<uint16_t>(value);

                /* Set the argument for method call */
                auto msg = dbus->new_method_call(postCodeService, postCodeObjPath,
                                         postCodeInterface, "GetPostCodes");
                msg.append(bootCycleIndex);

                /* Get the post code of CurrentBootCycleIndex */
                auto reply = dbus->call(msg);
                reply.read(tmpBuffer);

                /* Get post code data */
                for (int i = 0; i < tmpBuffer.size(); i++)
                {
                    dataInfo.push_back(std::get<0>(tmpBuffer[i]));
                }
            }
            catch (const sdbusplus::exception::SdBusError& e)
            {
                sd_journal_print(LOG_ERR,"IPMI GetPostCode Failed in call method, %s\n",e.what());
                return ipmi::responseUnspecifiedError();
            }
            return ipmi::responseSuccess(dataInfo);

        default:

            std::stringstream ss;
            ss << "Unknown subcommand : " << std::hex << std::setfill('0') << std::setw(2) << +param;
            std::string ss_str = ss.str();
            phosphor::logging::log<phosphor::logging::level::ERR>(ss_str.c_str());

            return ipmi::responseParmOutOfRange();
            break;
    }
    return ipmi::responseParmOutOfRange();
}


ipmi::RspType<uint16_t> ipmiStorageGetSELTimeUtcOffset()
{
   /* TODO: For now, the SEL time stamp is based on UTC time,
    * so return 0x0000 as offset. Might need to change once
    * supporting zones in SEL time stamps
    */

    uint16_t utcOffset = 0x0000;
    return ipmi::responseSuccess(utcOffset);
}

ipmi::RspType<std::vector<uint8_t>> ipmiGetDevGUID()
{
    std::vector<uint8_t> dataInfo;
    std::ifstream file;
    dataInfo.clear();

    try
    {
        file.open("/etc/system_fru.txt", std::ios::in);
        if (!file.is_open())
            throw std::runtime_error("Unable to open FRU file");

        for (std::string line; std::getline(file, line);)
        {
            if (line.find("GUID: ") != std::string::npos)
            {
                line.erase(0, 6); // This leaves GUID only
                line.erase(std::remove(line.begin(), line.end(), '-'), line.end());
                std::cerr << __FUNCTION__ << " GUID found:" << line << "\n";

                if (line.length() != 32)
                {
                    throw std::runtime_error("Wrong GUID length");
                }

                int i;
                for(i = 0; i<line.length(); i += 2)
                {
                    uint8_t byte;
                    if (sscanf(line.c_str() + i, "%2hhx", &byte) != 1)
                    {
                        throw std::runtime_error("Unable to parse GUID");
                    }
                    dataInfo.push_back(byte);
                }
                file.close();
                return ipmi::responseSuccess(dataInfo);
            }
            else
                continue;
        }
        file.close();
        throw std::runtime_error("No GUID in FRU");
    }
    catch (const std::exception& e)
    {
        std::cerr << __FUNCTION__ << " Error:" << e.what() << "\n";
        dataInfo.clear();
        if (file.is_open())
            file.close();
        for(int i=0; i<16; i++)
            dataInfo.push_back(0);
        return ipmi::responseSuccess(dataInfo);
    }

    return ipmi::responseSuccess(dataInfo);
}

/*
 * Get CPLD version
 * Request  raw 0x38 0x76
 * Response  15 19
 */
ipmi::RspType<std::vector<uint8_t>> ipmiOEMGetMBCPLDVersion()
{
    const uint8_t majorReg = 0x00;
    const uint8_t minorReg = 0x01;

    // FIXME TODO Need to distinguish between platforms
    static constexpr int i2cBusNumber = 31;
    // below given is 7bit address. Its 8bit addr is 0x2E
    static constexpr int i2cSlaveAddress = 0x17;

    try
    {
        I2CFile cpldDev(i2cBusNumber, i2cSlaveAddress, O_RDWR | O_CLOEXEC);
        uint8_t majorVer = cpldDev.i2cReadByteData(majorReg);
        uint8_t minorVer = cpldDev.i2cReadByteData(minorReg);

        // Major and Minor versions should be binary encoded strings.
        std::vector<std::uint8_t> version;
        version.push_back(majorVer);
        version.push_back(minorVer);

        return ipmi::responseSuccess(version);
    }
    catch (const std::exception& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Exception caught in readVersionFromCPLD.",
            phosphor::logging::entry("MSG=%s", e.what()));
        return ipmi::responseUnspecifiedError();
    }
}

ipmi::RspType<uint8_t> ipmiOEMChangeNVMECoolingSetpoint(uint8_t setpoint)
{
    constexpr size_t maxStepwisePoints = 5;

    struct PidsStruct {
        double reading[maxStepwisePoints];
    };

    struct JsonStruct {
        std::string name;
        PidsStruct pidsstruct;
    };

    int oldCoolingSetPoint = 0;
    uint8_t response = 0x00;

    std::ifstream input("/usr/share/swampd/config.json");
    nlohmann::json j;
    input >> j;

    JsonStruct jsonstruct;

    auto zones = j["zones"];
    std::ofstream output("/usr/share/swampd/config.json");

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

                if(oldCoolingSetPoint == setpoint)
                {
                    reading.value().at("1") = oldCoolingSetPoint;
                }
                else
                {
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

    std::cerr << "Updating cooling setpoint from: " << oldCoolingSetPoint
        << " to " << unsigned(setpoint) << "\n";
    output << std::setw(4) << j;

    output.close();
    input.close();


    if(oldCoolingSetPoint == setpoint)
    {
        std::cerr << "No need to restart PID control service" << "\n";
        response = 0x00;

        return ipmi::responseSuccess(response);
    }
    else
    {
        char command[100];
        std::stringstream cmd;
        std::string cmd_str;

        cmd << "systemctl restart --no-block phosphor-pid-control.service";
        std::cerr << cmd.str() << "\n";
        cmd_str = cmd.str();

        try
        {
          memset(command,0,sizeof(command));
          std::copy(cmd_str.begin(), cmd_str.end(), command);
          system(command);
          response = 0x01;

        }
        catch (std::system_error&)
        {
            std::cerr << "Error, not ready on shared memory" << "\n";
            return ipmi::responseUnspecifiedError();
        }

        return ipmi::responseSuccess(response);
    }

    return ipmi::responseUnspecifiedError();
}

/*
 * JBOD HDD issue fix
 * 0x38 0x78 <0-255 duration in ms>
 * BMC_RST_I2C_DEV_1 1-0-1 pulse
 */
ipmi::RspType<std::vector<uint8_t>> ipmiOEMsetGPIOvalue(uint8_t duration)
{
    std::vector<uint8_t> response;

    auto gpio_e0 = gpiod::find_line("BMC_RST_I2C_DEV_1");

    if (!gpio_e0)
    {
        std::cerr << "Error, Can't find BMC_RST_I2C_DEV_1 GPIO line" << "\n";
        return ipmi::responseUnspecifiedError();
    }

    int value = 1;

    try
    {
        gpio_e0.request({"ipmid", gpiod::line_request::DIRECTION_OUTPUT,0}, value);
    }
    catch (std::system_error&)
    {
        std::cerr << "Error requesting GPIOE0 BMC_RST_I2C_DEV_1" << "\n";
        return ipmi::responseUnspecifiedError();
    }

    gpio_e0.set_value(1);
    response.push_back((uint8_t)gpio_e0.get_value());

    gpio_e0.set_value(0);
    response.push_back((uint8_t)gpio_e0.get_value());

    usleep(static_cast <unsigned int> (duration*1000));

    gpio_e0.set_value(1);
    response.push_back((uint8_t)gpio_e0.get_value());

    gpio_e0.release();

    return ipmi::responseSuccess(response);
}

ipmi::RspType<std::vector<uint8_t>> ipmiOEMcheckRedriverSettings()
{
    std::vector<std::string> cmd;

    cmd.push_back("i2cdump -y 14 0x58 > /var/log/58_settings.dump");
    cmd.push_back("i2cdump -y 14 0x5a > /var/log/5a_settings.dump");
    cmd.push_back("i2cdump -y 14 0x5c > /var/log/5c_settings.dump");
    cmd.push_back("i2cdump -y 14 0x5e > /var/log/5e_settings.dump");

    for (auto c : cmd)
    {
        int ret = 0;
        ret = system(c.c_str());

        if(ret < 0)
        {
            std::cerr << "Redriver addr is wrong, or does not exist on i2cline" << "\n";
            return ipmi::responseUnspecifiedError();
        }
    }

    std::stringstream ss;
    std::string diffcmd;
    int ret = 0;

    std::vector<std::string> files = {"58_settings", "5a_settings", "5c_settings", "5e_settings"};

    for (auto filename : files)
    {
        ss.str("");

        ss << "diff /var/log/";
        ss << filename;
        ss << ".dump";
        ss << " /usr/share/openrack/";
        ss << filename;
        ss << ".set";

        ss << " > ";
        ss << "/tmp/";
        ss << filename;
        ss << ".diff";

        std::cerr << ss.str() << "\n";
        diffcmd = ss.str();

        ret = system(diffcmd.c_str());

        if(ret < 0)
        {
            std::cerr << "Diff CMD failed" << "\n";
            return ipmi::responseUnspecifiedError();
        }
    }

    std::vector<uint8_t> response;

    const fs::path file1("/tmp/58_settings.diff");
    const fs::path file2("/tmp/5a_settings.diff");
    const fs::path file3("/tmp/5c_settings.diff");
    const fs::path file4("/tmp/5e_settings.diff");

    std::vector<fs::path> diffiles = { "/tmp/58_settings.diff", "/tmp/5a_settings.diff" , "/tmp/5c_settings.diff", "/tmp/5e_settings.diff" };
    std::vector<uint8_t> redriveraddr = { 0x58, 0x5a, 0x5c, 0x5e };
    for(int i=0; i < 4; i++)
    {
        if (fs::is_empty(diffiles[i]))
        {
            response.push_back(redriveraddr[i]);
            response.push_back(0x00);
        }
        else
        {
            response.push_back(redriveraddr[i]);
            response.push_back(0x01);
        }
    }

    return ipmi::responseSuccess(response);
}

/*
 * Get DCDC vendor
 * 0x38 0x80
 * example usage ipmitool raw 0x38 0x80
 * Response: (as response to 0x99 PMBUS command)
 *           <06 44 45> - Delta
 *           <04 42 45> - Bel Power
 *           <08 41 72> - Artesyn
 *           <00 00 00> - Unknown
 */
ipmi::RspType<std::vector<uint8_t>> ipmiOEMGetDCDCVendorID()
{
    std::string dcdc;
    std::ifstream file;
    file.open("/etc/dcdc_model.txt", std::ios::in);

    if (!file.is_open())
    {
        throw std::runtime_error("Unable to open /etc/dcdc_model.txt file");
    }

    while (getline(file, dcdc))
    {
        std::stringstream iss(dcdc);
        break;
    }

    std::cerr << dcdc << "\n";

    std::vector<uint8_t> response;

    if (dcdc == "delta")
    {
        response = { 0x06, 0x44, 0x45 };
    }
    else if (dcdc == "bel")
    {
        response = { 0x04, 0x42, 0x45 };
    }
    else if (dcdc == "artesyn")
    {
        response = { 0x08, 0x41, 0x72 };
    }
    else
    {
        response = { 0x00, 0x00, 0x00 };
    }

    return ipmi::responseSuccess(response);
}

/*
 * Set Re-Driver register values
 * 0x38 0x72 <bus> <addr> <reg> <regValue>
 * example usage ipmitool raw 0x38 0x72 6 0x5c 0x06 0x24
 * Response: <00> - success
 *           <01> - write failed
 */

ipmi::RspType<uint8_t> ipmiOEMsetRedriversValues(uint8_t bus, uint8_t addr, uint8_t reg, uint8_t regValue)
{
    std::string cmdWrite;
    std::string cmdRead;
    std::stringstream ss;
    std::stringstream sss;

    std::ifstream file;
    file.open("/tmp/redriver_register_value.txt", std::ios::in);

    uint8_t response = 0x00;

    int ret = 0;

    ss << "i2cset -y";
    ss << std::hex << " 0x"<< +bus;
    ss << " 0x";
    ss << +addr;
    ss << " 0x";
    ss << +reg;
    ss << " 0x";
    ss << +regValue;

    std::cerr << ss.str() << "\n";
    cmdWrite = ss.str();

    ret = system(cmdWrite.c_str());

    if(ret < 0)
    {
        std::cerr << "i2cset write data to redrivers failed!" << "\n";
        response = 0x01;
    }

    sss << "i2cget -y";
    sss << std::hex << " 0x"<< +bus;
    sss << " 0x";
    sss << +addr;
    sss << " 0x";
    sss << +reg;
    sss << " > /tmp/redriver_register_value.txt";

    std::cerr << sss.str() << "\n";
    cmdRead = sss.str();

    ret = system(cmdRead.c_str());

    if(ret < 0)
    {
        std::cerr << "i2cset read data from redrivers failed!" << "\n";
        response = 0x01;
    }

    std::string tmpRead;
    std::stringstream iss;
    std::string tmpWriteToString;

    file >> tmpRead;

    iss << "0x"<< std::setw(2) << std::hex << (unsigned)regValue;
    tmpWriteToString = iss.str();

    if (tmpRead != tmpWriteToString)
    {
        std::cerr << "Write value and read value are not equal!" << "\n";
        response = 0x01;
    }

    file.close();

    return ipmi::responseSuccess(response);
}

/*
 * Run RMT Force mode
 * 0x38 0x81
 * example usage ipmitool raw 0x38 0x81
 * Response: <01> - success
 */

ipmi::RspType<uint8_t> ipmiRunRmtForce()
{
    uint8_t response;
    char command[100];
    std::stringstream cmd;
    std::string cmd_str;

    cmd << "/usr/sbin/rmtparser -f run &";
    std::cerr << cmd.str() << "\n";
    cmd_str = cmd.str();

    try
    {
        memset(command,0,sizeof(command));
        std::copy(cmd_str.begin(), cmd_str.end(), command);
        system(command);
        response = 0x01;
    }
    catch (std::system_error&)
    {
        std::cerr << "Can't run RMT service in FORCE mode!" << "\n";
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess(response);
}

/*
 * For passing DRAM B/w from host to BMC
 * 0x38 0x82 vector<uint8_t>
 * example usage ipmitool raw 0x38 0x82 0x37 0x6b 0xe
 * Response: <00> - success
 *           <01> - invalid amount of byte sent
 */

bool isEmpty(std::ifstream& pFile)
{
    return pFile.peek() == std::ifstream::traits_type::eof();
}

ipmi::RspType<uint8_t> ipmiPassMLCresults(std::vector<uint8_t> values)
{
    uint8_t response = 0x00;
    std::stringstream stream;
    int x;
    nlohmann::json j;
    long res = 0;
    std::ofstream outfile;

    if (values.size() == 2)
    {
        res = (res << 8) + values[0];
        res = (res << 8) + values[1];
    }
    else if (values.size() == 3)
    {
        res = (res << 8) + values[0];
        res = (res << 8) + values[1];
        res = (res << 4) + values[2];
    }
    else
    {
       response = 0x01;
       std::cerr << "Invalid byte amount sent" << "\n";
       return ipmi::responseSuccess(response);
    }

    stream << res;
    stream >> std::dec >> x;
    std::cerr << "MLC: " << x << " MB/s" << "\n";

    try
    {
        outfile.open("/var/lib/ipmi/bw.txt", std::ios_base::app); // append instead of overwrite
        outfile << res << "\n";
    }
    catch(nlohmann::json::exception &e)
    {
        std::cerr << "Error in JSON object caught!" << "\n";
        outfile.close();
        return ipmi::responseUnspecifiedError();
    }

    outfile.close();

    return ipmi::responseSuccess(response);
}

/*
 * Save configuration to persistent storage
 * Perform factory reset
 * 0x38 0x83 [ 0xDE 0xFA ]
 * 0xDE 0xFA params cause factory reset to be executed
 * example usage ipmitool raw 0x38 0x83
 * Response: <01> - success
 */
ipmi::RspType<std::vector<uint8_t>> ipmiSaveConfigToPersistent(std::vector<uint8_t> params)
{
    std::vector<uint8_t> response;
    char command[100];
    std::stringstream cmd;
    std::string cmd_str;

    // Check if factory reset is requested
    if (params.size() == 2 && params[0] == 0xde && params[1] == 0xfa)
    {
        std::cerr << "Performing factory reset\n";
        cmd << "/bin/sh -c 'source /usr/share/openrack/functions; factory_reset &'";
        cmd_str = cmd.str();
        response = { 0x01, 0x7e, 0xb0, 0x07 };
    } else
    {
        std::cerr << "Trying to save config to persistent storage.\n";
        cmd << "/usr/sbin/config-saver &";
        cmd_str = cmd.str();
        response = { 0x01 };
    }

    try
    {
        memset(command,0,sizeof(command));
        std::copy(cmd_str.begin(), cmd_str.end(), command);
        system(command);
    }
    catch (std::system_error&)
    {
        std::cerr << "Command failed:" << cmd_str << "\n";
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess(response);
}

/*
 * Start USBNET to host.
 * 0x38 0x84 <vlan_hi> <vlan_lo> [IP1.IP2.IP3.IP4 NETMASK]
 *           where VLAN_ID = vlan_hi * 256 + vlan_lo
 *           VLAN_ID 0 means bring up interface that
 *           connected to BMC (BMC-HOST connection)
 *           Only HTTPS(TCP/443) and ICMP are allowed
 *           through this interface.
 *           BMC IP address/netmask are optional.
 *           Hardcoded config is 192.168.255.254/24
 *           There is no DHCP server so host must
 *           set IP on its' side accrodingly.
 * example usage ipmitool raw 0x38 0x84 0 0 192 168 1 254 18
 * Response: <00> - success
 *           <01> - write failed
 */
ipmi::RspType<uint8_t> ipmiOEMStartUSBNet(std::vector<uint8_t> params)
{
    uint8_t response = 0x01;
    uint16_t vlan_id;
    char command[100];
    std::stringstream cmd;
    std::string cmd_str;

    if (params.size() < 2 || params.size() > 7)
    {
        return ipmi::responseReqDataLenInvalid();
    }

    vlan_id = params[0] * 256 + params[1];
    vlan_id &= 0xfff; // VLAN ID is 12-bits wide
    cmd << "/usr/sbin/usbnet on " << +vlan_id;
    if (params.size() == 7)
    {
        if (params[6] > 32)
        {
            return ipmi::responseParmOutOfRange();
        }

        cmd << " " << +params[2] << "."
                   << +params[3] << "."
                   << +params[4] << "."
                   << +params[5] << " "
                   << +params[6];
    }
    cmd << " &";
    std::cerr << "Starting USBNET:" << cmd.str() << "\n";
    cmd_str = cmd.str();

    try
    {
        memset(command,0,sizeof(command));
        std::copy(cmd_str.begin(), cmd_str.end(), command);
        system(command);
        response = 0x00;
    }
    catch (std::system_error&)
    {
        std::cerr << "Unable to start USBNET.\n";
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess(response);
}

/*
 * Stop USBNET to host.
 * 0x38 0x85
 * example usage ipmitool raw 0x38 0x85
 * Response: <00> - success
 *           <01> - write failed
 */
ipmi::RspType<uint8_t> ipmiOEMStopUSBNet()
{
    uint8_t response  = 0x01;
    char command[100];
    std::stringstream cmd;
    std::string cmd_str;

    std::cerr << "Stopping USBNET \n";
    cmd << "/usr/sbin/usbnet off &";
    cmd_str = cmd.str();

    try
    {
        memset(command,0,sizeof(command));
        std::copy(cmd_str.begin(), cmd_str.end(), command);
        system(command);
        response = 0x00;
    }
    catch (std::system_error&)
    {
        std::cerr << "Unable to stop USBNET.\n";
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess(response);
}

/*
 * Start/stop logging to remote trapdoor
 * 0x38 0x87 [cmd] [proto]
 *            cmd:
 *                1 - start
 *                0 - stop
 *                no cmd - get current status
 *            proto:
 *                0 - udp (default)
 *                1 - tcp
 * example usage ipmitool raw 0x38 0x87 1
 * Response: <00> - success
 *           <01> - write failed
 */
ipmi::RspType<uint8_t> ipmiOEMStartTrapdoor(std::vector<uint8_t> cmd)
{
    uint8_t response  = 0x01;
    char command[100];
    std::string cmdStr;
    const std::string fileConf("/etc/rsyslog.d/trapdoor.conf");
    const std::string fileTmpl("/etc/rsyslog.d/trapdoor.");
    std::string strLog;
    std::string strProto("udp");

    if (cmd.size() > 2)
    {
        return ipmi::responseReqDataLenInvalid();
    }

    if (cmd.size() == 0)
    {
        std::cerr << __FUNCTION__ << " No cmd given\n";
        std::ifstream f(fileConf.c_str());
        if (f.good())
        {
            f.close();
            return ipmi::responseSuccess(1);
        }
        else
        {
            return ipmi::responseSuccess(0);
        }
    }

    if (cmd.size() == 2 && cmd[1] == 1)
    {
        strProto = "tcp";
    }

    if (cmd[0] == 1)
    {
        cmdStr = "/bin/cp " + fileTmpl + strProto + " " + fileConf;
        strLog = "Starting trapdoor logging with " + fileTmpl + strProto + "\n";
    } else if (cmd[0] == 0)
    {
        cmdStr = "/bin/rm -f " + fileConf;
        strLog = "Stopping trapdoor logging\n";
    } else
    {
        return ipmi::responseUnspecifiedError();
    }

    try
    {
        std::cerr << __FUNCTION__ << " " << strLog << "\n";
        memset(command,0,sizeof(command));
        std::copy(cmdStr.begin(), cmdStr.end(), command);
        system(command);

        cmdStr = "/bin/systemctl --no-block restart rsyslog.service syslog.socket";
        memset(command,0,sizeof(command));
        std::copy(cmdStr.begin(), cmdStr.end(), command);
        system(command);

        response = 0x00;
    }
    catch (std::system_error&)
    {
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess(response);
}

/*
 * Start/Stop/get status of dropbear
 * 0x38 0x88 [cmd]
 *            cmd:
 *                1 - start
 *                0 - stop
 *                no cmd - get current status
 *            status:
 *                0 - disabled
 *                1 - enabled
 * example usage ipmitool raw 0x38 0x88 1
 * Response: <00> - dropbear is running
 *           <01> - dropbear is stopped
 *           <255> - command failed
 */
ipmi::RspType<uint8_t> ipmiOEMControlDropbear(std::vector<uint8_t> cmd)
{
    uint8_t response  = 0x01;
    char command[100];
    std::string cmd1;
    const std::string pidFile("/var/run/dropbear.pid");

    if (cmd.size() > 1)
    {
        return ipmi::responseReqDataLenInvalid();
    }

    if (cmd.size() == 0)
    {
        std::cerr << __FUNCTION__ << " No cmd given, returning status\n";
        std::ifstream f(pidFile.c_str());
        if (f.good())
        {
            f.close();
            return ipmi::responseSuccess(1);
        }
        else
        {
            return ipmi::responseSuccess(0);
        }
    }

    if (cmd[0] == 0)
    {
        cmd1 = "systemctl stop --no-block dropbear";
        response = 0;
    }
    else if (cmd[0] == 1)
    {
        cmd1 = "systemctl start --no-block dropbear";
        response = 1;
    }
    else
    {
        return ipmi::responseParmOutOfRange();
    }

    try
    {
        std::cerr << __FUNCTION__ << " running " << cmd1 << "\n";
        memset(command,0,sizeof(command));
        std::copy(cmd1.begin(), cmd1.end(), command);
        system(command);
    }
    catch (std::system_error&)
    {
        response = 0xff;
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess(response);
}

/*
 * Force password update
 * 0x38 0x89
 * example usage ipmitool raw 0x38 0x89
 * Response: <00> - success
 *           <01> - write failed
 */
ipmi::RspType<uint8_t> ipmiOEMSyncPassForce()
{
    uint8_t response  = 0x01;
    char command[100];
    std::string cmd("systemctl start --no-block syncpass.service &");

    try
    {
        std::cerr << __FUNCTION__ << " Synchronizing password\n";
        memset(command,0,sizeof(command));
        std::copy(cmd.begin(), cmd.end(), command);
        system(command);
        response = 0x00;
    }
    catch (std::system_error&)
    {
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess(response);
}

/*
 * Enable/Disable syncpass syncpass/checkpass services
 * 0x38 0x8A [0/1]
 *           0 - disable
 *           1 - enable service and force password update
 * example usage ipmitool raw 0x38 0x8a 1
 * Response: <00> - success, service disabled
 *           <01> - success, service enabled
 *           <ff> - error
 */
ipmi::RspType<uint8_t> ipmiOEMSyncPassService(std::vector<uint8_t> params)
{
    uint8_t response  = 0x01;
    char command[100];
    std::string systemctl_cmd;
    std::string cmd;

    if (params.size() > 1)
    {
        return ipmi::responseReqDataLenInvalid();
    }

    if (params.size() == 0)
    {
        // Get service status
        // Check /etc/systemd/system/multi-user.target.wants/syncpass.timer file
        response = 0;
        if(fs::exists("/etc/systemd/system/multi-user.target.wants/syncpass.timer"))
        {
            response = 1;
        }
        return ipmi::responseSuccess(response);
    }

    // No need for extra check
    if (params[0] == 0)
    {
        systemctl_cmd = "disable";
        response = 0;
    } else if (params[0] == 1)
    {
        systemctl_cmd = "enable";
        response = 1;
    }
    else
    {
        ipmi::responseParmOutOfRange();
    }

    try
    {
        std::cerr << __FUNCTION__ << " Setting syncpass timer to " << systemctl_cmd << ".\n";
        cmd = "systemctl " + systemctl_cmd + " --no-block syncpass.timer checkpass.timer &";
        memset(command,0,sizeof(command));
        std::copy(cmd.begin(), cmd.end(), command);
        system(command);
        if (systemctl_cmd == "enable")
        {
            ipmiOEMSyncPassForce();
        }
    }
    catch (std::system_error&)
    {
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess(response);
}

/*
 * Get DIP switch values on GD2 JBOD
 * 0x38 0x8B
 * example usage ipmitool raw 0x38 0x8B
 * Response: byte with lower nibble equal
 * to GPIOR2-R5 lines state
 */
ipmi::RspType<uint8_t> ipmiOEMGD2GetDipSwitch()
{
    uint8_t response  = 0x00;

    try
    {
        for(int i=0; i<4; i++)
        {
            /* Line names are R2, R3, R4, R5 */
            std::stringstream ss;
            ss << "R" << std::to_string(i + 2);
            std::string gpioName = ss.str();
            auto gpio_line = gpiod::find_line(gpioName.c_str());
            gpio_line.request({"ipmid", gpiod::line_request::DIRECTION_INPUT,0});
            uint8_t gpio_state = (uint8_t) gpio_line.get_value();
            gpio_line.release();
            response += ((gpio_state & 0x01) << i);
        }
    }
    catch (std::system_error&)
    {
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess(response);
}

/*
 * Disable factory mode
 * 0x38 0x8C [1]
 *           1 - disable factory mode and reboot
 * example usage ipmitool raw 0x38 0x8c 1
 * Response: <00> - success, factory mode disabled
 *           <01> - success, factory mode enabled
 *           <ff> - error
 */
ipmi::RspType<uint8_t> ipmiOEMFactory(std::vector<uint8_t> params)
{
    uint8_t response  = 0x01;
    char command[100];
    std::string cmd;

    if (params.size() > 1)
    {
        return ipmi::responseReqDataLenInvalid();
    }

    if (params.size() == 0)
    {
        // Get status by checking /etc/factory_mode file
        response = 0;
        if(fs::exists("/var/run/factory_mode"))
        {
            response = 1;
        }
        return ipmi::responseSuccess(response);
    }

    // No need for extra check
    if (params[0] == 1)
    {
        response = 0;
    }
    else
    {
        ipmi::responseParmOutOfRange();
    }

    try
    {
        std::cerr << __FUNCTION__ << " Disabling factory mode.\n";

        cmd = "/bin/sh -c 'source /usr/share/openrack/functions; eeprom_dropfactory'";
        memset(command,0,sizeof(command));
        std::copy(cmd.begin(), cmd.end(), command);
        system(command);

        cmd = "/bin/rm -rf /etc/factory_mode || :";
        memset(command,0,sizeof(command));
        std::copy(cmd.begin(), cmd.end(), command);
        system(command);

        std::cerr << __FUNCTION__ << " Rebooting\n";
        cmd = "/sbin/reboot &";
        memset(command,0,sizeof(command));
        std::copy(cmd.begin(), cmd.end(), command);
        system(command);
    }
    catch (std::system_error&)
    {
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess(response);
}

/*
 * Enable/Disable cauth service
 * 0x38 0x8D [0/1]
 *           0 - disable service
 *           1 - enable service
 * example usage ipmitool raw 0x38 0x8d 1
 * Response: <00> - success, service disabled
 *           <01> - success, service enabled
 *           <ff> - error
 */
ipmi::RspType<uint8_t> ipmiOEMCauthService(std::vector<uint8_t> params)
{
    uint8_t response  = 0x01;
    char command[100];
    std::string systemctl_cmd;
    std::string cmd;

    if (params.size() > 1)
    {
        return ipmi::responseReqDataLenInvalid();
    }

    if (params.size() == 0)
    {
        // Get service status
        response = 0;
        if(fs::exists("/etc/systemd/system/multi-user.target.wants/cauth-updater.timer"))
        {
            response = 1;
        }
        return ipmi::responseSuccess(response);
    }

    // No need for extra check
    if (params[0] == 0)
    {
        systemctl_cmd = "disable";
        response = 0;
    } else if (params[0] == 1)
    {
        systemctl_cmd = "enable";
        response = 1;
    }
    else
    {
        ipmi::responseParmOutOfRange();
    }

    try
    {
        std::cerr << __FUNCTION__ << " Setting cauth-updater timer to " << systemctl_cmd << ".\n";
        cmd = "systemctl " + systemctl_cmd + " --no-block cauth-updater.timer &";
        memset(command,0,sizeof(command));
        std::copy(cmd.begin(), cmd.end(), command);
        system(command);
    }
    catch (std::system_error&)
    {
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess(response);
}


void register_netfn_yndx_oem()
{
    //Twitter
    //ipmi_register_callback(NETFUN_TWITTER_OEM, IPMI_CMD_ClearCmos, NULL, ipmiOpmaClearCmos, PRIVILEGE_ADMIN);
    //ipmi_register_callback(NETFUN_TWITTER_OEM, IPMI_CMD_PnmGetReading, NULL, ipmi_Pnm_GetReading, PRIVILEGE_ADMIN);
    //ipmi::registerOemHandler(ipmi::prioMax, 0x0019fd, IPMI_CMD_FanPwmDuty, ipmi::Privilege::Admin, ipmi_tyan_FanPwmDuty);
    //ipmi::registerOemHandler(ipmi::prioMax, 0x0019fd, IPMI_CMD_ManufactureMode, ipmi::Privilege::Admin, ipmi_tyan_ManufactureMode);
    //ipmi::registerOemHandler(ipmi::prioMax, 0x002b3d, 0x21, ipmi::Privilege::Admin, printTest);
    //ipmi::registerHandler(ipmi::prioOemBase, 0x32, 0x92, ipmi::Privilege::Admin, ipmi::printTest);
    //ipmi::registerOemHandler(ipmi::prioMax, IANA_TYAN, IPMI_CMD_FloorDuty, ipmi::Privilege::Admin, ipmi_tyan_FloorDuty);
    //ipmi::registerOemHandler(ipmi::prioMax, IANA_TYAN, IPMI_CMD_ConfigEccLeakyBucket, ipmi::Privilege::Admin, ipmi_tyan_ConfigEccLeakyBucket);
    //ipmi::registerOemHandler(ipmi::prioMax, IANA_TYAN, IPMI_CMD_gpioStatus, ipmi::Privilege::Admin, ipmi_tyan_getGpio);
    //ipmi::registerOemHandler(ipmi::prioMax, IANA_TYAN, IPMI_CMD_SetFruField, ipmi::Privilege::Admin, ipmi_setFruField);
    //ipmi::registerOemHandler(ipmi::prioMax, IANA_TYAN, IPMI_CMD_GetFruField, ipmi::Privilege::Admin, ipmi_getFruField);
    //ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_SendRawPeci, ipmi::Privilege::Admin, ipmi_sendRawPeci);
    //ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_SetService, ipmi::Privilege::Admin, ipmi_SetService);
    //ipmi::registerHandler(ipmi::prioMax, NETFUN_TWITTER_OEM, IPMI_CMD_GetService, ipmi::Privilege::Admin, ipmi_GetService);
    //Yandex
    ipmi::registerOemHandler(ipmi::prioMax, IANA_YNDX, IPMI_CMD_GET_BIOS_LOCK_STATUS, ipmi::Privilege::Admin, ipmi::ipmiOEMgetBiosLockStatus);
    ipmi::registerOemHandler(ipmi::prioMax, IANA_YNDX, 0x30, ipmi::Privilege::Admin, ipmi::ipmiSetFpLED);
    ipmi::registerOemHandler(ipmi::prioMax, IANA_YNDX, 0x31, ipmi::Privilege::Admin, ipmi::ipmiSetJBODLed);
    ipmi::registerHandler(ipmi::prioMax, NETFN_YANDEX_OEM, IPMI_CMD_GET_BMC_BASIC_INFO, ipmi::Privilege::Admin, ipmi::ipmigetNetworkData);
    ipmi::registerOemHandler(ipmi::prioMax, IANA_GBT, IPMI_CMD_GET_BUNCH_OF_DATA, ipmi::Privilege::Admin, ipmi::ipmiOEMgetBunchOfData);
    ipmi::registerOemHandler(ipmi::prioMax, IANA_GBT, IPMI_CMD_SMBIOS_DATA_READY_ON_SHARED_MEMORY,ipmi::Privilege::Admin, ipmi::ipmiOEMreadyOnShareMemory);
    ipmi::registerOemHandler(ipmi::prioMax, IANA_GBT, IPMI_CMD_SMBIOS_INT_FLAG,ipmi::Privilege::Admin, ipmi::ipmiOEMaskInternalFlag);
    ipmi::registerOemHandler(ipmi::prioMax, IANA_GBT, IPMI_CMD_UNK_1,ipmi::Privilege::Admin, ipmi::ipmiOEMcmdUnkOne);
    //ipmi::registerOemHandler(ipmi::prioMax, IANA_GBT, IPMI_CMD_ENABLE_DESABLE_SENSORS_POLLING, ipmi::Privilege::Admin, ipmi::ipmiOEMcmdEnableDesableSensorsPolling);
    ipmi::registerHandler(ipmi::prioMax, NETFN_YANDEX_OEM, IPMI_CMD_GET_MB_CPLD_FW_VERSION, ipmi::Privilege::Admin, ipmi::ipmiOEMGetMBCPLDVersion);
    ipmi::registerHandler(ipmi::prioMax, NETFN_YANDEX_OEM, IPMI_CMD_GET_ME_FW_VERSION, ipmi::Privilege::Admin, ipmi::ipmiOEMGetMeFwVersion);
    ipmi::registerHandler(ipmi::prioMax, NETFN_YANDEX_OEM, IPMI_CMD_GET_ME_STATUS, ipmi::Privilege::Admin, ipmi::ipmiOEMGetMeStatus);
    ipmi::registerHandler(ipmi::prioMax, NETFN_YANDEX_OEM, IPMI_CMD_CHECK_REDRIVER_SETTINGS, ipmi::Privilege::Admin, ipmi::ipmiOEMcheckRedriverSettings);
    ipmi::registerHandler(ipmi::prioMax, NETFN_YANDEX_OEM, IPMI_CMD_GET_DCDC_VENDOR_ID, ipmi::Privilege::Admin, ipmi::ipmiOEMGetDCDCVendorID);
    //ipmi::registerHandler(ipmi::prioMax, NETFN_YANDEX_OEM, IPMI_CMD_CHANGE_NVME_COOLING_SETPOINT, ipmi::Privilege::Admin, ipmi::ipmiOEMChangeNVMECoolingSetpoint);
    ipmi::registerHandler(ipmi::prioMax, NETFN_YANDEX_OEM, IPMI_CMD_RUN_RMT_FORCE, ipmi::Privilege::Admin, ipmi::ipmiRunRmtForce);
    ipmi::registerHandler(ipmi::prioMax, NETFN_YANDEX_OEM, IPMI_CMD_SAVE_CONFIG, ipmi::Privilege::Admin, ipmi::ipmiSaveConfigToPersistent);
    ipmi::registerHandler(ipmi::prioMax, NETFN_YANDEX_OEM, IPMI_CMD_PASS_MLC, ipmi::Privilege::Admin, ipmi::ipmiPassMLCresults);
    ipmi::registerHandler(ipmi::prioMax, NETFN_GBT_OEM, IPMI_CMD_GET_BIOS_POST, ipmi::Privilege::Admin, ipmi::ipmiGetPostCode);
    //IPMI SPEC
    ipmi::registerHandler(ipmi::prioMax, NETFUN_STORAGE, IPMI_CMD_GET_SEL_TIME_UTC_OFFSET, ipmi::Privilege::Admin, ipmi::ipmiStorageGetSELTimeUtcOffset);
    //ipmi::registerHandler(ipmi::prioMax, NETFUN_APP, IPMI__CMD_GET_DEV_GUID, ipmi::Privilege::Admin, ipmi::ipmiGetDevGUID);
    ipmi::registerHandler(ipmi::prioMax, NETFUN_APP, IPMI_CMD_GET_SYS_GUID, ipmi::Privilege::User, ipmi::ipmiGetDevGUID);
    ipmi::registerHandler(ipmi::prioMax, NETFN_YANDEX_OEM, IPMI_CMD_SET_GPIOE0, ipmi::Privilege::Admin, ipmi::ipmiOEMsetGPIOvalue);
    ipmi::registerHandler(ipmi::prioMax, NETFN_YANDEX_OEM, IPMI_CMD_SET_REDRIVERS, ipmi::Privilege::Admin, ipmi::ipmiOEMsetRedriversValues);

    ipmi::registerHandler(ipmi::prioMax, NETFN_YANDEX_OEM, IPMI_CMD_START_USBNET, ipmi::Privilege::Admin, ipmi::ipmiOEMStartUSBNet);
    ipmi::registerHandler(ipmi::prioMax, NETFN_YANDEX_OEM, IPMI_CMD_STOP_USBNET, ipmi::Privilege::Admin, ipmi::ipmiOEMStopUSBNet);

    ipmi::registerHandler(ipmi::prioMax, NETFN_YANDEX_OEM, IPMI_CMD_IDENTITY_CONTROL, ipmi::Privilege::Admin, ipmi::ipmiIdentityLedControl);

    ipmi::registerHandler(ipmi::prioMax, NETFN_YANDEX_OEM, IPMI_CMD_TRAPDOOR, ipmi::Privilege::Admin, ipmi::ipmiOEMStartTrapdoor);
    ipmi::registerHandler(ipmi::prioMax, NETFN_YANDEX_OEM, IPMI_CMD_DROPBEAR, ipmi::Privilege::Admin, ipmi::ipmiOEMControlDropbear);
    ipmi::registerHandler(ipmi::prioMax, NETFN_YANDEX_OEM, IPMI_CMD_SYNCPASS_FORCE, ipmi::Privilege::Admin, ipmi::ipmiOEMSyncPassForce);
    ipmi::registerHandler(ipmi::prioMax, NETFN_YANDEX_OEM, IPMI_CMD_SYNCPASS_SERVICE, ipmi::Privilege::Admin, ipmi::ipmiOEMSyncPassService);
    ipmi::registerHandler(ipmi::prioMax, NETFN_YANDEX_OEM, IPMI_CMD_GD2_DIP_SWITCH, ipmi::Privilege::Admin, ipmi::ipmiOEMGD2GetDipSwitch);
    ipmi::registerHandler(ipmi::prioMax, NETFN_YANDEX_OEM, IPMI_CMD_FACTORY, ipmi::Privilege::Admin, ipmi::ipmiOEMFactory);
    ipmi::registerHandler(ipmi::prioMax, NETFN_YANDEX_OEM, IPMI_CMD_CAUTH_SERVICE, ipmi::Privilege::Admin, ipmi::ipmiOEMCauthService);
}
}
