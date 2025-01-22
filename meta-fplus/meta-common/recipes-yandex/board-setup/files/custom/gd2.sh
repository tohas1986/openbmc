#!/bin/bash

. /usr/share/openrack/functions

echo GD2 JBOD detected, disabling services
_service_list="smbios-mdrv2.service "
_service_list="$_service_list xyz.openbmc_project.State.Boot.PostCode.service xyz.openbmc_project.VirtualMedia.service "
_service_list="$_service_list start-ipkvm.service phosphor-ipmi-kcs@ipmi_kcs4.service phosphor-ipmi-kcs@ipmi_kcs3.service"
_service_list="$_service_list lpcsnoop.service phosphor-watchdog.service rmtparser.service"
_service_list="$_service_list usbnet-start.service"
systemctl disable --no-block $_service_list
systemctl stop --no-block $_service_list

# Let's check JBOD mode; Options SATA/EXPANDER
gd2mode=$(strings -n 1 $FRU_PATH | grep "Y4J-G" | head -n1)

echo Instantiating I2C devices
# INA219 sensors
echo ina219 0x41 > /sys/bus/i2c/devices/i2c-14/new_device || :
echo 500 > /sys/bus/i2c/devices/14-0041/hwmon/hwmon*/shunt_resistor || :
#echo ina219 0x40 > /sys/bus/i2c/devices/i2c-14/new_device || :
#echo 1000 > /sys/bus/i2c/devices/14-0040/hwmon/hwmon*/shunt_resistor || :

# LM5066i05 sensors
lm5066i_setmask 24 0x40 || :
echo lm5066i 0x40 > /sys/bus/i2c/devices/i2c-24/new_device || :
lm5066i_setmask 24 0x41 || :
echo lm5066i 0x41 > /sys/bus/i2c/devices/i2c-24/new_device || :
if [ "$gd2mode" = "Y4J-GE1-TY25-3Q1" ]; then
	echo "Found Expander mode JBOD: $gd2mode"

	echo ina219 0x44 > /sys/bus/i2c/devices/i2c-14/new_device || :
	echo ina219 0x45 > /sys/bus/i2c/devices/i2c-14/new_device || :
	echo 1000 > /sys/bus/i2c/devices/14-0044/hwmon/hwmon*/shunt_resistor || :
	echo 1000 > /sys/bus/i2c/devices/14-0045/hwmon/hwmon*/shunt_resistor || :
	echo ina219 0x44 > /sys/bus/i2c/devices/i2c-15/new_device || :
	echo ina219 0x45 > /sys/bus/i2c/devices/i2c-15/new_device || :
	echo 1000 > /sys/bus/i2c/devices/15-0044/hwmon/hwmon*/shunt_resistor || :
	echo 1000 > /sys/bus/i2c/devices/15-0045/hwmon/hwmon*/shunt_resistor || :
	echo ina219 0x44 > /sys/bus/i2c/devices/i2c-16/new_device || :
	echo ina219 0x45 > /sys/bus/i2c/devices/i2c-16/new_device || :
	echo 1000 > /sys/bus/i2c/devices/16-0044/hwmon/hwmon*/shunt_resistor || :
	echo 1000 > /sys/bus/i2c/devices/16-0045/hwmon/hwmon*/shunt_resistor || :
	echo ina219 0x44 > /sys/bus/i2c/devices/i2c-17/new_device || :
	echo ina219 0x45 > /sys/bus/i2c/devices/i2c-17/new_device || :
	echo 1000 > /sys/bus/i2c/devices/17-0044/hwmon/hwmon*/shunt_resistor || :
	echo 1000 > /sys/bus/i2c/devices/17-0045/hwmon/hwmon*/shunt_resistor || :
	echo ina219 0x44 > /sys/bus/i2c/devices/i2c-18/new_device || :
	echo ina219 0x45 > /sys/bus/i2c/devices/i2c-18/new_device || :
	echo 1000 > /sys/bus/i2c/devices/18-0044/hwmon/hwmon*/shunt_resistor || :
	echo 1000 > /sys/bus/i2c/devices/18-0045/hwmon/hwmon*/shunt_resistor || :
	echo ina219 0x44 > /sys/bus/i2c/devices/i2c-19/new_device || :
	echo ina219 0x45 > /sys/bus/i2c/devices/i2c-19/new_device || :
	echo 1000 > /sys/bus/i2c/devices/19-0044/hwmon/hwmon*/shunt_resistor || :
	echo 1000 > /sys/bus/i2c/devices/19-0045/hwmon/hwmon*/shunt_resistor || :
	echo ina219 0x44 > /sys/bus/i2c/devices/i2c-20/new_device || :
	echo ina219 0x45 > /sys/bus/i2c/devices/i2c-20/new_device || :
	echo 1000 > /sys/bus/i2c/devices/20-0044/hwmon/hwmon*/shunt_resistor || :
	echo 1000 > /sys/bus/i2c/devices/20-0045/hwmon/hwmon*/shunt_resistor || :
	echo ina219 0x44 > /sys/bus/i2c/devices/i2c-21/new_device || :
	echo ina219 0x45 > /sys/bus/i2c/devices/i2c-21/new_device || :
	echo 1000 > /sys/bus/i2c/devices/21-0044/hwmon/hwmon*/shunt_resistor || :
	echo 1000 > /sys/bus/i2c/devices/21-0045/hwmon/hwmon*/shunt_resistor || :
fi
if [ "$gd2mode" = "Y4J-GD1-TY25-3Q0" ]; then
	echo "Found SATA mode JBOD: $gd2mode"

	echo ina219 0x44 > /sys/bus/i2c/devices/i2c-14/new_device || :
	echo ina219 0x45 > /sys/bus/i2c/devices/i2c-14/new_device || :
	echo 1000 > /sys/bus/i2c/devices/14-0044/hwmon/hwmon*/shunt_resistor || :
	echo 1000 > /sys/bus/i2c/devices/14-0045/hwmon/hwmon*/shunt_resistor || :
	echo ina219 0x44 > /sys/bus/i2c/devices/i2c-15/new_device || :
	echo ina219 0x45 > /sys/bus/i2c/devices/i2c-15/new_device || :
	echo 1000 > /sys/bus/i2c/devices/15-0044/hwmon/hwmon*/shunt_resistor || :
	echo 1000 > /sys/bus/i2c/devices/15-0045/hwmon/hwmon*/shunt_resistor || :
	echo ina219 0x44 > /sys/bus/i2c/devices/i2c-16/new_device || :
	echo ina219 0x45 > /sys/bus/i2c/devices/i2c-16/new_device || :
	echo 1000 > /sys/bus/i2c/devices/16-0044/hwmon/hwmon*/shunt_resistor || :
	echo 1000 > /sys/bus/i2c/devices/16-0045/hwmon/hwmon*/shunt_resistor || :
	echo ina219 0x44 > /sys/bus/i2c/devices/i2c-17/new_device || :
	echo ina219 0x45 > /sys/bus/i2c/devices/i2c-17/new_device || :
	echo 1000 > /sys/bus/i2c/devices/17-0044/hwmon/hwmon*/shunt_resistor || :
	echo 1000 > /sys/bus/i2c/devices/17-0045/hwmon/hwmon*/shunt_resistor || :
	echo ina219 0x44 > /sys/bus/i2c/devices/i2c-18/new_device || :
	echo ina219 0x45 > /sys/bus/i2c/devices/i2c-18/new_device || :
	echo 1000 > /sys/bus/i2c/devices/18-0044/hwmon/hwmon*/shunt_resistor || :
	echo 1000 > /sys/bus/i2c/devices/18-0045/hwmon/hwmon*/shunt_resistor || :

	echo ina219 0x44 > /sys/bus/i2c/devices/i2c-26/new_device || :
	echo ina219 0x45 > /sys/bus/i2c/devices/i2c-26/new_device || :
	echo 1000 > /sys/bus/i2c/devices/26-0044/hwmon/hwmon*/shunt_resistor || :
	echo 1000 > /sys/bus/i2c/devices/26-0045/hwmon/hwmon*/shunt_resistor || :
	echo ina219 0x44 > /sys/bus/i2c/devices/i2c-27/new_device || :
	echo ina219 0x45 > /sys/bus/i2c/devices/i2c-27/new_device || :
	echo 1000 > /sys/bus/i2c/devices/27-0044/hwmon/hwmon*/shunt_resistor || :
	echo 1000 > /sys/bus/i2c/devices/27-0045/hwmon/hwmon*/shunt_resistor || :
	echo ina219 0x44 > /sys/bus/i2c/devices/i2c-28/new_device || :
	echo ina219 0x45 > /sys/bus/i2c/devices/i2c-28/new_device || :
	echo 1000 > /sys/bus/i2c/devices/28-0044/hwmon/hwmon*/shunt_resistor || :
	echo 1000 > /sys/bus/i2c/devices/28-0045/hwmon/hwmon*/shunt_resistor || :
	echo ina219 0x44 > /sys/bus/i2c/devices/i2c-29/new_device || :
	echo ina219 0x45 > /sys/bus/i2c/devices/i2c-29/new_device || :
	echo 1000 > /sys/bus/i2c/devices/29-0044/hwmon/hwmon*/shunt_resistor || :
	echo 1000 > /sys/bus/i2c/devices/29-0045/hwmon/hwmon*/shunt_resistor || :
	echo ina219 0x44 > /sys/bus/i2c/devices/i2c-30/new_device || :
	echo ina219 0x45 > /sys/bus/i2c/devices/i2c-30/new_device || :
	echo 1000 > /sys/bus/i2c/devices/30-0044/hwmon/hwmon*/shunt_resistor || :
	echo 1000 > /sys/bus/i2c/devices/30-0045/hwmon/hwmon*/shunt_resistor || :
fi
