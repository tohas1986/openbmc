#!/bin/sh

if [ "$1" == "on" ]; then
    echo "Power Good, Enable GPU Power Good"
    echo "Set GPU state On"

    gpioset $(gpiofind HGX_BASE_PWR_EN)=1
    sleep 3

    power=$(ipmitool power status)

    sleep 0.8

    if [ "$power" == "Chassis Power is on" ]; then
        echo "GPU PWR Good PEX_RST Up"
        gpioset $(gpiofind HGX_PEX_RST_R_N0)=1
        gpioset $(gpiofind HGX_PEX_RST_R_N1)=1
        gpioset $(gpiofind HGX_PEX_RST_R_N2)=1

        sleep 0.1

        echo "Set SW Persistant to On"
        gpioset $(gpiofind SW_PERST_N0)=1
        gpioset $(gpiofind SW_PERST_N1)=1
        gpioset $(gpiofind SW_PERST_N2)=1
        gpioset $(gpiofind SW_PERST_N3)=1
        
        sleep 1

        echo "Run Arbitration service in order to set SMBPBI interface to Host BMC"
        systemctl start setHostBmcArbitration.service

    else
        echo "GPU PWR Not Good PEX_RST Down"
        gpioset $(gpiofind HGX_PEX_RST_R_N0)=0
        gpioset $(gpiofind HGX_PEX_RST_R_N1)=0
        gpioset $(gpiofind HGX_PEX_RST_R_N2)=0

        echo "Set SW Persistant to Off"
        gpioset $(gpiofind SW_PERST_N0)=0
        gpioset $(gpiofind SW_PERST_N1)=0
        gpioset $(gpiofind SW_PERST_N2)=0
        gpioset $(gpiofind SW_PERST_N3)=0
    fi
elif [ "$1" == "off" ]; then
    echo "Set GPU state Off"

    echo "GPU PWR Not Good PEX_RST Down"
    gpioset $(gpiofind HGX_PEX_RST_R_N0)=0
    gpioset $(gpiofind HGX_PEX_RST_R_N1)=0
    gpioset $(gpiofind HGX_PEX_RST_R_N2)=0

    echo "Set PWR_EN off"
    gpioset $(gpiofind HGX_BASE_PWR_EN)=0

    echo "Set SW Persistant to Off"
    gpioset $(gpiofind SW_PERST_N0)=0
    gpioset $(gpiofind SW_PERST_N1)=0
    gpioset $(gpiofind SW_PERST_N2)=0
    gpioset $(gpiofind SW_PERST_N3)=0
fi
