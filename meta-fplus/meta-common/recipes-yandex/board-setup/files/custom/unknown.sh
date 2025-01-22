#!/bin/bash
echo "Undetected motherboard, reduced mode, stopping services"
echo "Stopping services"
_service_list="smbios-mdrv2.service start-ipkvm.service phosphor-pid-control.service"
_service_list="$_service_list xyz.openbmc_project.State.Boot.PostCode.service xyz.openbmc_project.VirtualMedia.service "
_service_list="$_service_list lpcsnoop.service phosphor-watchdog.service mapper-wait@-xyz-openbmc_project-inventory-system-board.service"
_service_list="$_service_list obmc-console@ttyS2.service obmc-console@ttyS3.service obmc-console@ttyVUART0.service inventory-job.service"
_service_list="$_service_list usbnet-start.service"
systemctl stop --no-block $_service_list
systemctl disable --no-block inventory-job.service || :
