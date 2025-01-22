#!/bin/bash

. /usr/share/openrack/functions

echo MY81 Rack 3.0 detected, disabling services
_service_list="phosphor-pid-control.service"
systemctl disable --no-block $_service_list
systemctl stop --no-block $_service_list
echo Enabling VGA
enable_vga_scu
echo "Setting UART1<->UART4 routing"
uart_set_route uart1 uart4 || :
echo INA219
echo ina219 0x45 > /sys/bus/i2c/devices/i2c-6/new_device || :
sleep 1
echo 167 > /sys/bus/i2c/devices/6-0045/hwmon/hwmon*/shunt_resistor || :
echo 0 > /sys/bus/i2c/devices/6-0045/hwmon/hwmon*/linear_k || :
