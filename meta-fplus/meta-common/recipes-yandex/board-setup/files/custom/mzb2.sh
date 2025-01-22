#!/bin/bash

. /usr/share/openrack/functions

echo MZB2 detected
echo "Setting UART1<->UART4 routing"
uart_set_route uart1 uart4 || :
echo Resetting I2C muxes
gpioset $(gpiofind BMC_RST_I2C_DEV_1)=0 || :
sleep 0.5 || :
gpioset $(gpiofind BMC_RST_I2C_DEV_1)=1 || :
sleep 0.5 || :
echo MZB2 does not have IPMB, disabling...
_service_list="ipmb.service"
systemctl disable --no-block $_service_list
systemctl stop --no-block $_service_list
echo Setting up LM5066i05
# LM5066i05 sensors

lm5066i_add 23 0x40
lm5066i_add 50 0x40

echo DCDC: Getting vendor
dcdc_getvendor 23 0x60 || :
echo DCDC: Resetting alerts
i2ctransfer -y 23 w1@0x60 0x03 || :
echo Disable SPI pass-through
disable_spi_passthrough
echo Disable GPIOS1 bounce
devmem 0x1e6e208c 32 0x20ed || :
