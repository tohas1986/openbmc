/*
 *  nvmecooler.hpp
 *
 *  Created on: May 18, 2021
 *  Author: Nikita Vedeneev
 *  Company: Yandex LLC
 */

#define NVME_SSD_SLAVE_ADDRESS 0x6a
#define NVME_SSD_VPD_SLAVE_ADDRESS 0x53
#define MAX_I2C_BUS 30

constexpr size_t maxStepwisePoints = 5;

struct PidsStruct
{
    double reading[maxStepwisePoints];
};

struct JsonStruct
{
    std::string name;
    PidsStruct pidsstruct;
};

struct NVMETableStruct
{
    std::map<std::string, int> Models;
};

/*
* Structure for keeping nvme data required by nvme monitoring
*/

struct NVMeData
{
    bool present;               /* Whether or not the nvme is present  */
    std::string vendor;         /* The nvme manufacturer  */
    std::string serialNumber;   /* The nvme serial number  */
    std::string modelNumber;    /* The nvme model number   */
    std::string smartWarnings;  /* Indicates smart warnings for the state  */
    std::string statusFlags;    /* Indicates the status of the drives  */
    std::string driveLifeUsed;  /* A vendor specific estimate of the percentage  */
    int8_t sensorValue;         /* Sensor value, if sensor value didn't be
                                   update, means sensor failure, default set to
                                   129(0x81) accroding to NVMe-MI SPEC*/
    int8_t wcTemp;              /* Indicates over temperature warning threshold.
                                   This is intended to initially match the temperature
                                   reported in the WCTEMP field in the NVMe Identify
                                   Controller data structure */
};
