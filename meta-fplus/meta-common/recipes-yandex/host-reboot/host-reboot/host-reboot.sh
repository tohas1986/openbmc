#!/bin/sh

echo "Check the BMC reboot reason"
rbt_reason=$(devmem 0x1E785010)
echo $rbt_reason
echo "Is the reason AC power on"

if [ "$rbt_reason" == "0x00000000" ]; then
	echo "Reset the host started"
    ipmitool power cycle
fi

echo "The service succesfully finished"