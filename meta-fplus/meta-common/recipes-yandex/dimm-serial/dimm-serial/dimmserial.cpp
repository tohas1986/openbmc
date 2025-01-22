
#include <boost/asio/io_service.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus/match.hpp>
#include <systemd/sd-bus.h>
#include <algorithm>
#include "dimmserial.hpp"

uint8_t fileData[2048];


std::string getDimmVendor(int dimmNumber)
{
    std::string result = "";
    
    //sdbus entities initialization
    int r;
    sd_bus *bus = NULL;
    sd_bus_error error = SD_BUS_ERROR_NULL;
	sd_bus_message *reply = NULL;
    char *readval;
    std::string dimmPath = "/xyz/openbmc_project/inventory/system/chassis/motherboard/dimm" + std::to_string(dimmNumber);

    //Read a Manufacturer fields from dbus
    r = sd_bus_default_system(&bus);
	if(r < 0) {
        std::cerr << "Cannot bind a sdbus lib\n";
        return result;
    }

	r = sd_bus_call_method(bus, "xyz.openbmc_project.Smbios.MDR_V2", dimmPath.c_str(), 
		  "org.freedesktop.DBus.Properties", "Get", &error, &reply, "ss", 
		  "xyz.openbmc_project.Inventory.Decorator.Asset", "Manufacturer" );
    if(r < 0) {
        std::cerr << "Cannot call dbus\n";
        return result;
    }

    r = sd_bus_message_enter_container(reply, SD_BUS_TYPE_VARIANT, NULL);
    if(r < 0) {
        std::cerr << "Cannot create container\n";
        return result;
    }

    r = sd_bus_message_read(reply, "s", &readval);
    if(r < 0) {
        std::cerr << "Cannot read dbus value\n";
        return result;
    }
    else {
        std::string completeResult (readval);
        r = sd_bus_message_exit_container(reply);
        if(r < 0) {
            std::cerr << "Cannot close the container\n";
             return result;
        }
        sd_bus_unref(bus);

        return completeResult;
    }
}

std::string getDimmSerialNumFromDbus(int dimmNumber)
{
    std::string result = "";
    
    //sdbus entities initialization
    int r;
    sd_bus *bus = NULL;
    sd_bus_error error = SD_BUS_ERROR_NULL;
	sd_bus_message *reply = NULL;
    char *readval;
    std::string dimmPath = "/xyz/openbmc_project/inventory/system/chassis/motherboard/dimm" + std::to_string(dimmNumber);

    //Read a Manufacturer fields from dbus
    r = sd_bus_default_system(&bus);
	if(r < 0) {
        std::cerr << "Cannot bind a sdbus lib\n";
        return result;
    }

	r = sd_bus_call_method(bus, "xyz.openbmc_project.Smbios.MDR_V2", dimmPath.c_str(), 
		  "org.freedesktop.DBus.Properties", "Get", &error, &reply, "ss", 
		  "xyz.openbmc_project.Inventory.Decorator.Asset", "SerialNumber" );
    if(r < 0) {
        std::cerr << "Cannot call dbus\n";
        return result;
    }

    r = sd_bus_message_enter_container(reply, SD_BUS_TYPE_VARIANT, NULL);
    if(r < 0) {
        std::cerr << "Cannot create container\n";
        return result;
    }

    r = sd_bus_message_read(reply, "s", &readval);
    if(r < 0) {
        std::cerr << "Cannot read dbus value\n";
        return result;
    }
    else {
        std::string completeResult (readval);
        r = sd_bus_message_exit_container(reply);
        if(r < 0) {
            std::cerr << "Cannot close the container\n";
             return result;
        }
        sd_bus_unref(bus);

        return completeResult;
    }
}

int getBusNumber (int num)
{
    if (num>=0 && num <=3) return 24;
    if (num>=4 && num <=7) return 25;
    if (num>=8 && num <=11) return 26;
    if (num>=12 && num <=15) return 27;
}

std::string getBusAddr (int num)
{
    std::string res = "";

    if (num == 0 || num == 4 || num == 8 || num == 12) 
    {
        res = "50";
    }
    if (num == 1 || num == 5 || num == 9 || num == 13) 
    {
        res = "52";
    }
    if (num == 2 || num == 6 || num == 10 || num == 14) 
    {
        res = "54";
    }
    if (num == 3 || num == 7 || num == 11 || num == 15) 
    {
        res = "56";
    }

    return res;
}

void readFile(const char* filename, uint8_t *Data)
{
	// open the file:
	std::streampos fileSize;
	std::ifstream file(filename, std::ios::binary);

	// get its size:
	file.seekg(0, std::ios::end);
	fileSize = file.tellg();
	file.seekg(0, std::ios::beg);

	// read the data:
	//std::vector<BYTE> Data(fileSize);
	file.read((char*)&Data[0], 2048);
	//return fileData;
}

std::string conv2Hex(uint8_t decimal)
{
	int remainder, product = 1;
	std::string hex_dec = "";
	while (decimal != 0) {
		remainder = decimal % 16;
		char ch;
		if (remainder >= 10)
			ch = remainder + 55;
		else
			ch = remainder + 48;
		hex_dec += ch;
  
		decimal = decimal / 16;
		product *= 10;
	}
	std::reverse(hex_dec.begin(), hex_dec.end());
	return hex_dec;
}

int spdParse (int num)
{
    char *part0;
    uint8_t part1;
    uint8_t part2;
    std::string part3;
    std::string hex_part1;
    std::string hex_part2;

    std::string longSerial;

    //sdbus entities initialization
    int r;
    sd_bus *bus = NULL;
    sd_bus_error error = SD_BUS_ERROR_NULL;
	sd_bus_message *reply = NULL;
    char *readval;
    std::string dimmPath = "/xyz/openbmc_project/inventory/system/chassis/motherboard/dimm" + std::to_string(num);

    readFile("/tmp/spd", fileData);
	uint8_t* dataIn = (uint8_t*)fileData;
	auto spdData = reinterpret_cast<struct spd_ddr4*>(dataIn);
    part0 = (char*)spdData->spdSpecificMfgData;
    part1 = (uint8_t)spdData->spdMfgyear;
    hex_part1 = conv2Hex(part1);
    //part1 = part1 && 0x0000FFFF;
    part2 = (uint8_t)spdData->spdMfgWeek;
    hex_part2= conv2Hex(part2);
    
    part3 = getDimmSerialNumFromDbus(num);//(char*)spdData->spdSerialNum;
    if (part3.length() > 10) {
        //Serial Already changed
        return 1;
    }

    std::string part0String;
    for (int i = 0; i <= 6; i++) part0String += part0[i];

    longSerial = part0String + hex_part1[1] + hex_part2 + part3;//std::to_string(part1) + std::to_string(part2) + part3;

    std::cerr << "DEBUG: longSerial  " << longSerial << "\n";

    r = sd_bus_default_system(&bus);
	if(r < 0) {
        std::cerr << "Cannot bind a sdbus lib HERE\n";
        return -1;
    }

    r = sd_bus_set_property(bus, "xyz.openbmc_project.Smbios.MDR_V2", dimmPath.c_str(), "xyz.openbmc_project.Inventory.Decorator.Asset", "SerialNumber", &error, "s", longSerial.c_str());
    if(r < 0) {
        std::cerr << "DEBUG: Failed to set value \n";
        return -1;
    }
    else {
        sd_bus_unref(bus);
    }

    return 1;
}

int createLongSerial(int dimmNumber)
{
    int busNumber;
    std::string i2cAddr;
    std::string spdCmd;
    std::string spdRead;
    std::string spdDelete;

    busNumber = getBusNumber(dimmNumber);
    i2cAddr = getBusAddr(dimmNumber);

    spdCmd = "echo ee1004 0x" + i2cAddr + " > /sys/bus/i2c/devices/i2c-" + std::to_string(busNumber) + "/new_device";
    std::cerr << "DEBUG: spdCmd is " << spdCmd << "\n";
    system(spdCmd.c_str());

    system("sleep 0.5");

    spdRead = "dd if=/sys/bus/i2c/devices/i2c-" + std::to_string(busNumber) + "/" + std::to_string(busNumber) + "-00" +i2cAddr + "/eeprom of=/tmp/spd";
    std::cerr << "DEBUG: spdRead is " << spdRead << "\n";
    system(spdRead.c_str());
    system("sleep 0.5");

    if (spdParse(dimmNumber) < 0) 
    {
        std::cerr << "DEBUG: parser does not work \n";
        return -1;
    }

    spdDelete = "echo 0x" + i2cAddr + " > /sys/bus/i2c/devices/i2c-" + std::to_string(busNumber) + "/delete_device";
    system(spdDelete.c_str());

    return 0;
}

int main(int argc, char* argv[])
{
    std::string dimmVendor;
    int status;

    std::cerr << "The service status is " << +SRV_STARTED << "\n";

    for (int i=0; i<=15; i++) {
        dimmVendor = getDimmVendor(i);
        if (dimmVendor == "Samsung") {
            //Start to make long S/N
            status = createLongSerial(i);
            if (status < 0) 
            {
                std::cerr << "Something wrong with  " << +i << " DIMM\n";
            }
        }
        else {
            std::cerr << "Does not need to modify serial \n";
        }
    }

    return 0;
}