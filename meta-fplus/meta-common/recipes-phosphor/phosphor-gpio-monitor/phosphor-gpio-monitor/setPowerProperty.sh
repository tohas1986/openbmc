#!/bin/sh

if [ "$1" == "on" ]; then
    echo "Computing node connected, Power Good"
    echo "Set JBOG Power State On"
    busctl set-property "xyz.openbmc_project.JBOGPowerState" "/xyz/openbmc_project/sensors/jbog/JBOGPWR" "xyz.openbmc_project.JBOG.Value" "Value" s "On"
    ipmitool power on
    sleep 3
    systemctl start --no-block SetGPUPowerGoodPropertyOn.service
elif [ "$1" == "off" ]; then
    echo "Computing Node disconnected or powered off, Power Off"
    echo "Set JBOG Power State Off"
    busctl set-property "xyz.openbmc_project.JBOGPowerState" "/xyz/openbmc_project/sensors/jbog/JBOGPWR" "xyz.openbmc_project.JBOG.Value" "Value" s "Off"
    ipmitool power off
    systemctl start --no-block SetGPUPowerGoodPropertyOff.service
fi
