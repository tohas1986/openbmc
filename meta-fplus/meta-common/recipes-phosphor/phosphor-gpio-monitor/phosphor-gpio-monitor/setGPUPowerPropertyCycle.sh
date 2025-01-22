#!/bin/sh

if [ "$1" == "cycle" ]; then

    sleep 5
    BoardState=$(busctl get-property xyz.openbmc_project.JBOGPowerState /xyz/openbmc_project/sensors/jbog/JBOGPWR xyz.openbmc_project.JBOG.Value Value | cut -d' ' -f2 | sed 's/\"//g')
    #Look for MB power Good
    if [ "$BoardState" == "Off" ]; then
        echo "Board power Off, exitting"
        exit 0
    fi

    echo "Set GPU state Cycle"
    busctl set-property "xyz.openbmc_project.JBOGPowerState" "/xyz/openbmc_project/sensors/jbog/JBOGPWR" "xyz.openbmc_project.JBOG.Value" "Value" s "Off"
    ipmitool power off

    sleep 0.2

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

    sleep 2
    busctl set-property "xyz.openbmc_project.JBOGPowerState" "/xyz/openbmc_project/sensors/jbog/JBOGPWR" "xyz.openbmc_project.JBOG.Value" "Value" s "On"
    ipmitool power on

    echo "Power Good, Enable GPU Power Good"
    echo "Set GPU state On"

    gpioset $(gpiofind HGX_BASE_PWR_EN)=1
    sleep 3

    echo "GPU PWR Good PEX_RST Up"
    gpioset $(gpiofind HGX_PEX_RST_R_N0)=1
    gpioset $(gpiofind HGX_PEX_RST_R_N1)=1
    gpioset $(gpiofind HGX_PEX_RST_R_N2)=1

    power=$(ipmitool power status)

    sleep 0.8

    if [ "$power" == "Chassis Power is on" ]; then
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
fi
