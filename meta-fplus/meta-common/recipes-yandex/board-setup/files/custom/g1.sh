#!/bin/bash

. /usr/share/openrack/functions

echo G1 JBOG detected, disabling services

_service_list="smbios-mdrv2.service "
_service_list="$_service_list xyz.openbmc_project.State.Boot.PostCode.service xyz.openbmc_project.VirtualMedia.service "
_service_list="$_service_list start-ipkvm.service phosphor-ipmi-kcs@ipmi_kcs4.service phosphor-ipmi-kcs@ipmi_kcs3.service"
_service_list="$_service_list lpcsnoop.service phosphor-watchdog.service rmtparser.service"
_service_list="$_service_list phosphor-pid-control nvmecooler.service nvmecooler.timer com.intel.crashdump.service"
_service_list="$_service_list usbnet-start.service"
systemctl disable --no-block $_service_list
systemctl stop --no-block $_service_list

echo "Load fans' HWMON driver"
echo adt7462 0x58 > /sys/bus/i2c/devices/i2c-14/new_device
echo adt7462 0x58 > /sys/bus/i2c/devices/i2c-15/new_device
fanpdb1="/sys/bus/i2c/devices/14-0058/hwmon/hwmon*"
fanpdb2="/sys/bus/i2c/devices/15-0058/hwmon/hwmon*"
echo 0 > $fanpdb1/setup_complete || :
echo 0 > $fanpdb2/setup_complete || :

for i in 1 2 3 4; do echo 1 > $fanpdb1/pwm${i}_enable; echo 1 > $fanpdb1/fan${i}_presence; echo 1 > $fanpdb1/fan${i}_tach_enable; done
for i in 1 2 3 4; do echo 1 > $fanpdb2/pwm${i}_enable; echo 1 > $fanpdb2/fan${i}_presence; echo 1 > $fanpdb2/fan${i}_tach_enable; done

for i in 1 2 3 4; do echo 0 > $fanpdb1/pwm${i}_auto_point1_pwm; echo 255 > $fanpdb1/pwm${i}_auto_point2_pwm; done
for i in 1 2 3 4; do echo 0 > $fanpdb2/pwm${i}_auto_point1_pwm; echo 255 > $fanpdb2/pwm${i}_auto_point2_pwm; done

echo 1 > $fanpdb1/force_pwm_max || :
echo 1 > $fanpdb2/force_pwm_max || :

echo 1 > $fanpdb1/setup_complete || :
echo 1 > $fanpdb2/setup_complete || :

echo "Enable PID control"
systemctl enable --no-block phosphor-pid-control || :
systemctl start --no-block phosphor-pid-control || :

echo "Setting up LM5066i05"
lm5066i_setmask 14 0x40 || :
echo lm5066i 0x40 > /sys/bus/i2c/devices/i2c-14/new_device || :

lm5066i_setmask 15 0x40 || :
echo lm5066i 0x40 > /sys/bus/i2c/devices/i2c-15/new_device || :

lm5066i_setmask 30 0x40 || :
echo lm5066i 0x40 > /sys/bus/i2c/devices/i2c-30/new_device || :

