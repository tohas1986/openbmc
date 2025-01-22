#!/bin/bash

. /usr/share/openrack/functions

echo AI1 detected
echo "Disabling UART routing"
uart_set_route uart1 io1 || :
uart_set_route uart2 io2 || :
uart_set_route uart3 io3 || :
uart_set_route uart4 io4 || :
echo Setting up LM5066i05
lm5066i_add 0 0x15
echo Setting INA for GPU kit NODES
echo ina219 0x40 > /sys/bus/i2c/devices/i2c-51/new_device || :
echo ina219 0x44 > /sys/bus/i2c/devices/i2c-52/new_device || :
echo DCDC: Getting vendor
dcdc_getvendor 0 0x60 || :
echo DCDC: Resetting alerts
i2ctransfer -y 0 w1@0x60 0x03 || :
#echo "Initializing re-drivers"
#ai1_init_redrivers 0x5c 1
#ai1_init_redrivers 0x5d 2
#ai1_init_redrivers 0x5e 3
#ai1_init_redrivers 0x5f 4
echo Disable SPI pass-through
disable_spi_passthrough
echo Disable VUART
devmem 0x1e787020 32 0 || :
