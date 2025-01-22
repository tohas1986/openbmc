/*
 *  hwwbus-hb.hpp
 *
 *  Created on: April 05, 2022
 *  Author: Konstantin Klubnichkin
 *  Company: Yandex LLC
 */

#define NVME_SSD_SLAVE_ADDRESS 0x6a
#define NVME_SSD_VPD_SLAVE_ADDRESS 0x53
#define MAX_I2C_BUS 30

constexpr size_t maxStepwisePoints = 5;

/*
* Structure of UDP packet for heartbeat
* https://wiki.yandex-team.ru/haas/emergency/heartbeat/
*/

struct HeartbeatPacket
{
    uint16_t    version;    /* 0x0001 in current release (forever) */
    uint16_t    interval;   /* 0x003c, 60s */
    char        mac[13];    /* IPMI MAC address without colons (0cc47a1a79ce\0) */
    uint8_t     gap[16];    /* 16 zeroes gap */
    char        json[256];  /* JSON string with ipmi and ts mandatory fields
                            * Example: {"ipmi":"0015b2a7246b","uptime":29960203,"ts":1644935775}
                            */
} __attribute__((packed));;
