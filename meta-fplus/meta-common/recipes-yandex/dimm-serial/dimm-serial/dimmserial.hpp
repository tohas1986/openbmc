#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <regex>
#include <sstream>
#include <map>
#include <iomanip>

#define SRV_STARTED 0xAA

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;

struct spd_ddr4 {
	BYTE spdBytesUsed;
	BYTE spdRevIn;
	BYTE spdBasicMemType;
	BYTE spdModuleType;
	BYTE spdBankData;
	BYTE spdRowCol;
	BYTE spdPackage;
	BYTE spdFeatures;
	BYTE spdThermalReserved;
	BYTE spdOtherFeatures;
	BYTE spdSecPackage;
	BYTE spdVoltage;
	BYTE spdOrganization;
	BYTE spdMemBus;
	BYTE spdThermSensor;
	BYTE spdExtModuleType;
	BYTE spdReserved1;
	BYTE spdMeasuredPs;
	BYTE spdMinCycleTime;
	BYTE spdMaxCycleTime;
	BYTE spdCAS0;
	BYTE spdCAS1;
	BYTE spdCAS2;
	BYTE spdCAS3;
	BYTE spdMinCASlat;
	BYTE spdMinRAStoCAS;
	BYTE spdMinRowPrecharge;
	BYTE spdUpperNibbles;
	BYTE spdMinTras;
	BYTE spdMinTrc;
	BYTE spdMinTrfc11;
	BYTE spdMinTrfc12;
	BYTE spdMinTrfc21;
	BYTE spdMinTrfc22;
	BYTE spdMinTrfc41;
	BYTE spdMinTrfc42;
	BYTE spdMinTfawLow;
	BYTE spdMinTfawHigh;
	BYTE spdMinTrrds;
	BYTE spdMinTrrdl;
	BYTE spdMinTccdl;
	BYTE spdTwrHigh;
	BYTE spdTwrLow;
	BYTE spdTwtrHigh;
	BYTE spdTwtrs;
	BYTE spdTwtrl;
	BYTE spdReserved2[14];
	BYTE spdConnector[18];
	BYTE spdReserved3[39];
	BYTE spdMinTccdl2;
	BYTE spdMinTrrdl2;
	BYTE spdMinTrrds2;
	BYTE spdActiveToActiveTrc;
	BYTE spdRowPrechargheTrp;
	BYTE spdRasToCasDelay;
	BYTE spdTaa;
	BYTE spdTckavgMax;
	BYTE spdTckavgMin;
	BYTE spdCRClow;
	BYTE spdCRChigh;
	BYTE spdModuleSpecificSection[64];
	BYTE spdHybridMemoryArchSpec[64];
	BYTE spdExtFunctions[64];
	BYTE spdManufacturer[2];
	BYTE spdMfgLocation;
	BYTE spdMfgyear;
	BYTE spdMfgWeek;
	BYTE spdSerialNum[4];
	BYTE spdPartNumber[20];
	BYTE spdModuleRevCode;
	BYTE spdManufacturerID[2];
	BYTE spdStepping;
	BYTE spdSpecificMfgData[29];
	BYTE spdReserved4[2];
	BYTE spdFree[128];
};

std::string getDimmVendor (int dimmNumber);
std::string getDimmSerialNumFromDbus (int dimmNumber);
std::string conv2Hex(uint8_t decimal);

int createLongSerial(int dimmNumber);
std::string getBusAddr (int num);
int getBusNumber (int num);
int spdParse (int num);

