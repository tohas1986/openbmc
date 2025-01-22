#!/bin/bash

. /usr/share/openrack/functions

echo "EVB2600 detected"
echo "Stopping services"
_service_list=""
_service_list="$_service_list xyz.openbmc_project.State.Boot.PostCode.service "
_service_list="$_service_list phosphor-watchdog.service mapper-wait@-xyz-openbmc_project-inventory-system-board.service"
_service_list="$_service_list inventory-job.service"
systemctl stop --no-block $_service_list
systemctl start --no-block start-ipkvm.service || :
#echo "Enabling EHCI on PCIe"
#devmem 0x1e6e2500 32 0x2060e2
