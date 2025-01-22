#include <stdint.h>
#include <ipmid/api.h>
#include <string.h>

enum ipmi_net_fns_oem
{
    NETFN_YANDEX_OEM = 0x38,
    NETFN_GBT_OEM = 0x32,
    NETFN_IPMI_SPEC = 0x0a,
    NETFN_IPMI_APP = 0x06,
};

#define IANA_GBT 0x003c0a
#define IANA_YNDX 0x002b3d
#define IANA_INTL 0x000157

enum ipmi_net_yndx_fns_oem_cmds
{
    //IPMI_CMD_FanPwmDuty = 0x05,
    //IPMI_CMD_ManufactureMode = 0x06,
    //IPMI_CMD_FloorDuty = 0x07,
    //IPMI_CMD_SetFruField = 0x0B,
    //IPMI_CMD_GetFruField = 0x0C,
    //IPMI_CMD_SetService = 0x0D,
    //IPMI_CMD_GetService = 0x0E,
    //IPMI_CMD_ConfigEccLeakyBucket = 0x1A,
    //IPMI_CMD_ClearCmos = 0x3A,
    //IPMI_CMD_gpioStatus = 0x41,
    //IPMI_CMD_PnmGetReading = 0xE2,
    //IPMI_CMD_SendRawPeci = 0xE6, //Intel RAW PECI NetFn=0x30, cmd = 0xE6
    //IPMI_CMD_RamdomDelayACRestorePowerON = 0x18,
    IPMI_CMD_LED_CONTROL = 0x10,
    IPMI_CMD_GET_BIOS_LOCK_STATUS = 0x21,
    IPMI_CMD_GET_BMC_BASIC_INFO = 0x30,
    IPMI_CMD_SET_REDRIVERS = 0x72,
    IPMI_CMD_GET_BIOS_POST = 0x73,
    IPMI_CMD_GET_ME_FW_VERSION = 0x74,
    IPMI_CMD_GET_ME_STATUS = 0x75,
    IPMI_CMD_GET_MB_CPLD_FW_VERSION = 0x76,
    IPMI_CMD_CHANGE_NVME_COOLING_SETPOINT = 0x77,
    IPMI_CMD_SET_GPIOE0 = 0x78,
    IPMI_CMD_CHECK_REDRIVER_SETTINGS = 0x79,
    IPMI_CMD_GET_DCDC_VENDOR_ID = 0x80,
    IPMI_CMD_RUN_RMT_FORCE = 0x81,
    IPMI_CMD_PASS_MLC = 0x82,
    IPMI_CMD_SAVE_CONFIG = 0x83,
    IPMI_CMD_START_USBNET = 0x84,
    IPMI_CMD_STOP_USBNET = 0x85,
    IPMI_CMD_IDENTITY_CONTROL = 0x86,
    IPMI_CMD_TRAPDOOR = 0x87,
    IPMI_CMD_DROPBEAR = 0x88,
    IPMI_CMD_SYNCPASS_FORCE = 0x89,
    IPMI_CMD_SYNCPASS_SERVICE = 0x8A,
    IPMI_CMD_GD2_DIP_SWITCH = 0x8B,
    IPMI_CMD_FACTORY = 0x8C,
    IPMI_CMD_CAUTH_SERVICE = 0x8D,
};

enum ipmi_net_gbt_fns_oem_cmds
{
    IPMI_CMD_GET_BUNCH_OF_DATA = 0x21,
    IPMI_CMD_SMBIOS_DATA_READY_ON_SHARED_MEMORY = 0x20,
    IPMI_CMD_SMBIOS_INT_FLAG = 0x11,
    IPMI_CMD_UNK_1 = 0x30,
    IPMI_CMD_ENABLE_DESABLE_SENSORS_POLLING = 0x10,
};

enum ipmi_net_spec_fns_oem_cmds
{
    IPMI_CMD_GET_SEL_TIME_UTC_OFFSET = 0x5c,
    IPMI_CMD_GET_DEV_GUID = 0x08,
    IPMI_CMD_GET_SYS_GUID = 0x37,
};
//YANDEX SUB COMMADS
#define SUB_CMD_GET_BIOS_LOCK_STATUS 0x24
#define SUB_CMD_FLIP_BIOS_LOCK_OVERRIDE 0x25
#define SUB_CMD_GET_BMC_BASIC_INFO 0x02
#define SUB_CMD_GET_TRAY_TYPE 0x09

//GBT SUB COMMANDS
#define SUB_CMD_GET_SHARED_MEMORY_ADDRESS 0x0a
#define SUB_CMD_SMBIOS_DATA_READY_ON_SHARED_MEMORY 0x0b
#define SUB_CMD_INT_FLAG 0x16
#define SUB_CMD_PLATFORM_FLAG 0x01
#define UNK_CMD_1 0x52
#define UNK_CMD_2 0x24
#define SEB_CMD_GET_BIOS_POST 0x00
#define SUB_SENSORS_POLLING_CMD 0x04
#define SUB_GPU_POLLING_CMD 0x05

//GBT SUB SUB COMMANDS
#define SUB_CMD_GET_BMC_IP 0x04
#define SUB_CMD_GET_BMC_IPv6 0x02
#define SUB_CMD_GET_BMC_IPv6_LOCAL 0x04
#define SUB_CMD_GET_BIOS_VERSION 0x19

struct PnmGetReadingRequest
{
    uint8_t type;
    uint8_t reserved1;
    uint8_t reserved2;
};

typedef struct
{
    uint8_t nmVersion : 4, // Node Manager Version
        dcmiVersion : 4;   // DCMI Version
    uint8_t b : 4,         // BCD encoded Build Number - tens
        a : 4,             // BCD encoded Build Number - hundreds
        patch : 4,         // BCD encoded Patch Number
        c : 4;             // BCD encoded Build Number - digits
    uint8_t imageFlags;
} __attribute__((packed)) ipmiFwVerAux;

typedef struct
{
    uint8_t b0;
    uint8_t b1;
    uint8_t b2;
} __attribute__((packed)) ipmiIana;

/**
 * @brief Part of Get Device ID Command Response Payload
 */
typedef struct
{
    uint8_t fwMajorRev : 7,  // Binary encoded Major Version
        inUpgrade : 1;       // In Upgrade State
    uint8_t fwHotfixRev : 4, // BCD encoded Hotfix Version
        fwMinorRev : 4;      // BCD encoded Minor Version
} __attribute__((packed)) ipmiFwVerMajorMinor;

/**
 * @brief Get Device ID Command Full Response Payload
 */
typedef struct
{
    uint8_t deviceId;                 // Device ID
    uint8_t deviceRev : 4,            // Device Revision
        reserved0 : 3,                // Reserved bits
        sdrPresent : 1;               // SDR State
    ipmiFwVerMajorMinor fwMajorMinor; // Major and Minor Version
    uint8_t ipmiVersion;   // BCD encoded IPMI Version, reversed digit order
    uint8_t featureMask;   // Bitmask of supported features
    ipmiIana ianaId;       // Manufacturers ID
    uint8_t prodIdMinor;   // Product ID Minor Version
    uint8_t prodIdMajor;   // Product ID Major Version
    ipmiFwVerAux fwVerAux; // NmVersion, Build Number etc.
} __attribute__((packed)) ipmiGetDeviceIdResp;
