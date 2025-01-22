#!/bin/bash

. /usr/share/openrack/functions

echo GD1 JBOD detected, disabling services
_service_list="smbios-mdrv2.service "
_service_list="$_service_list xyz.openbmc_project.State.Boot.PostCode.service xyz.openbmc_project.VirtualMedia.service "
_service_list="$_service_list start-ipkvm.service phosphor-ipmi-kcs@ipmi_kcs4.service phosphor-ipmi-kcs@ipmi_kcs3.service"
_service_list="$_service_list lpcsnoop.service phosphor-watchdog.service rmtparser.service"
_service_list="$_service_list usbnet-start.service"
systemctl disable --no-block $_service_list
systemctl stop --no-block $_service_list

echo Instantiating I2C devices
echo adm1278 0x40 > /sys/bus/i2c/devices/i2c-14/new_device || :
echo adm1278 0x41 > /sys/bus/i2c/devices/i2c-14/new_device || :
echo pmbus 0x4b > /sys/bus/i2c/devices/i2c-15/new_device || :
lm5066i_setmask 16 0x40 || :
echo lm5066i 0x40 > /sys/bus/i2c/devices/i2c-16/new_device || :
lm5066i_setmask 16 0x41 || :
echo lm5066i 0x41 > /sys/bus/i2c/devices/i2c-16/new_device || :
echo tmp75 0x4c > /sys/bus/i2c/devices/i2c-17/new_device || :
echo tmp75 0x4d > /sys/bus/i2c/devices/i2c-17/new_device || :
echo tmp75 0x4e > /sys/bus/i2c/devices/i2c-17/new_device || :
echo tmp75 0x4f > /sys/bus/i2c/devices/i2c-17/new_device || :
echo tmp75 0x48 > /sys/bus/i2c/devices/i2c-7/new_device || :
# JC42 (CAT34TS02) on bridge boards
echo jc42 0x18 > /sys/bus/i2c/devices/i2c-4/new_device || :
echo jc42 0x18 > /sys/bus/i2c/devices/i2c-6/new_device || :
# INA219 sensors
echo ina219 0x44 > /sys/bus/i2c/devices/i2c-8/new_device || :
echo ina219 0x45 > /sys/bus/i2c/devices/i2c-8/new_device || :
echo 500 > /sys/bus/i2c/devices/8-0044/hwmon/hwmon*/shunt_resistor || :
echo 500 > /sys/bus/i2c/devices/8-0045/hwmon/hwmon*/shunt_resistor || :
echo ina219 0x44 > /sys/bus/i2c/devices/i2c-9/new_device || :
echo ina219 0x45 > /sys/bus/i2c/devices/i2c-9/new_device || :
echo 500 > /sys/bus/i2c/devices/9-0044/hwmon/hwmon*/shunt_resistor || :
echo 500 > /sys/bus/i2c/devices/9-0045/hwmon/hwmon*/shunt_resistor || :
echo DCDC: Resetting alerts
i2ctransfer -y 15 w1@0x60 0x03 || :
