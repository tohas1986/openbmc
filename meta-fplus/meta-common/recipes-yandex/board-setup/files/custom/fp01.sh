#!/bin/bash

. /usr/share/openrack/functions

echo FP01 compute node detected
#echo Resetting I2C muxes
#gpioset $(gpiofind BMC_RST_I2C_DEV_1)=0 || :
#sleep 0.5 || :
#gpioset $(gpiofind BMC_RST_I2C_DEV_1)=1 || :
#sleep 0.5 || :
#echo Setting up LM5066i05
#lm5066i_add 23 0x40
#lm5066i_add 50 0x40
#echo DCDC: Getting vendor
#dcdc_getvendor 23 0x60 || :
#echo DCDC: Resetting alerts
#i2ctransfer -y 23 w1@0x60 0x03 || :
# INA219 sensors
#echo ina219 0x40 > /sys/bus/i2c/devices/i2c-14/new_device || :
#echo 600 > /sys/bus/i2c/devices/14-0040/hwmon/hwmon*/shunt_resistor || :
#echo Disable SPI pass-through
#disable_spi_passthrough
#echo "Setting UART1<->UART4 routing"
#uart_set_route uart1 uart4 || :
#echo Enabling rmt-service
#systemctl enable --no-block rmtparser.service || :
#systemctl start --no-block rmtparser.service || :
