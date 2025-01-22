#!/bin/sh

# Yandex version

setup_image()
{
	local storage="$1"
	local sz_mb="$2"
	# create the backing store
	dd if=/dev/zero of="$storage" bs=1M seek="$sz_mb" count=0 2>/dev/null
	# this shows up as 23FC-F676 in /dev/disk/by-uuid
	local diskid=0x23FCF676
	mkdosfs -n 'OPENBMC-FW' -i $diskid -I "$storage" >/dev/null 2>&1
}

mount_image()
{
	local storage="$1"
	local stormnt="$2"
	mkdir -p $stormnt || exit 1
	mount -o loop -t vfat "$storage" "$stormnt"
}

cleanup_image()
{
	local storage="$1"
	local stormnt="$2"
	umount -f "$stormnt"
	rm -f "$storage"
	rmdir "$stormnt"
}

ecm()
{
	local name="$1"
	local on="$2"
	local bmc_mac="$3"
	local host_mac="$4"
	if [ "$on" = "on" ]; then
		usb_insert "${name}" ecm "${bmc_mac}" "${host_mac}"
	elif [ "$on" = "off" ]; then
		usb_eject "${name}" ecm
	else
		echo "Unknown ecm command"
		usage
	fi
}

GADGET_BASE=/sys/kernel/config/usb_gadget

get_free_usb_port()
{
	local in_use="$(cat $GADGET_BASE/*/UDC)"
	in_use="$in_use 1e6a0000.usb-vhub:p1"
	for D in /sys/class/udc/*; do
		local port=$(echo $D | sed 's/\/sys\/class\/udc\///g')
		echo $in_use | grep -q $port && continue
		echo $port
		return 0
	done
	return 1
}

usb_ms_insert()
{
	usb_insert "$1" mass_storage "$2" "$3"
}

usb_ms_eject()
{
	usb_eject "$1" mass_storage
}

## $1:  device syspath to provide usb-gadget configure,
##	  e.g. functions/mass_storage.usb0/lun.0/
##
## $2:  optional usb gadget interface type, e.g. usb|usb-ro|hdd|cdrom.
##	  if $2 not specified or illegal, then using 'cdrom' as default
usb_set_interface_type()
{
	local usb_gadget_syspath="$1"
	local interface_type="${2:-'cdrom'}"
	# defining target variables to configure interface type
	local removable=
	local ro=
	local cdrom=
	if [ ! -d ${usb_gadget_syspath} ]; then
		echo "Device syspath ${usb_gadget_syspath} does not exist" >&2
		return 1
	fi
	case "${interface_type}" in
		hdd)
			removable=0
			ro=0
			cdrom=0
			;;
		usb)
			removable=1
			ro=0
			cdrom=0
			;;
		cdrom)
			removable=1
			ro=1
			cdrom=1
			;;
		usb-ro)
			removable=1
			ro=1
			cdrom=0
			;;
		*)
			echo "No mass-storage interface type specified or illegal" >&2
			echo "Configuring interface type to 'usb-ro'" >&2
			removable=1
			ro=1
			cdrom=0
			;;
	esac
	echo $removable > ${usb_gadget_syspath}/removable
	echo $ro > ${usb_gadget_syspath}/ro
	echo $cdrom > ${usb_gadget_syspath}/cdrom
}

## $1: device name, e.g. usb0, usb1
## $2: device type defined in kernel, e.g. mass_storage, ecm
## $3: optional storage file, e.g. /dev/nbd1, /tmp/boot.iso
## $4: optional mass storage interface type, e.g. usb|usb-ro|hdd|cdrom.
##	 if interface type not specified then using 'usb-ro' as defaul
usb_insert()
{
	local name="$1"
	local dev_type="$2"
	local storage="$3"
	local interface_type="${4:-'usb-ro'}"
	local bmc_mac="$3"
	local host_mac="$4"

	if [ -d $GADGET_BASE/"${name}" ]; then
		echo "Device "${name}" already exists" >&2
		return 1
	fi
	local gadget_base="$GADGET_BASE/$name"
	mkdir $gadget_base

	echo 0x1d6b > $gadget_base/idVendor	# Linux Foundation
	echo 0x0105 > $gadget_base/idProduct # FunctionFS Gadget
	mkdir $gadget_base/strings/0x409
	local machineid=$(cat /etc/machine-id)
	local data="OpenBMC USB gadget device serial number"
	local serial=$( echo -n "${machineid}${data}${machineid}" | \
		sha256sum | cut -b 0-12 )
	echo $serial > $gadget_base/strings/0x409/serialnumber
	echo OpenBMC > $gadget_base/strings/0x409/manufacturer
	echo "OpenBMC USB Device" > $gadget_base/strings/0x409/product

	local gadget_function="$gadget_base/functions/${dev_type}.${name}"
	mkdir $gadget_base/configs/c.1
	mkdir "${gadget_function}"
	if [ "${dev_type}" = "mass_storage" ]; then
		usb_set_interface_type "${gadget_function}/lun.0" "${interface_type}"
		echo "${storage}" > "${gadget_function}/lun.0/file"
	elif [ "${dev_type}" = "ecm" ]; then
		echo "${bmc_mac}" > "${gadget_function}/dev_addr"
		echo "${host_mac}" > "${gadget_function}/host_addr"
	fi
	mkdir $gadget_base/configs/c.1/strings/0x409

	echo "Conf 1" > $gadget_base/configs/c.1/strings/0x409/configuration
	echo 120 > $gadget_base/configs/c.1/MaxPower
	ln -s "${gadget_function}" $gadget_base/configs/c.1
	local dev=$(get_free_usb_port)
	echo $dev > $gadget_base/UDC
}

## $1: device name, e.g. usb0, usb1
## $2: device type defined in kernel, e.g. mass_storage, ecm
usb_eject()
{
	local name="$1"
	local dev_type="$2"
	local gadget_base="$GADGET_BASE/$name"

	[ ! -d "$gadget_base" ] && return 0
	echo '' > $gadget_base/UDC

	rm -f $gadget_base/configs/c.1/"${dev_type}"."${name}"
	rmdir $gadget_base/configs/c.1/strings/0x409
	rmdir $gadget_base/configs/c.1
	rmdir $gadget_base/functions/"${dev_type}"."${name}"
	rmdir $gadget_base/strings/0x409
	rmdir $gadget_base
}

usage()
{
	echo "Usage: $0 <action> ..."
	echo "	   $0 setup <file> <sizeMB>"
	echo "	   $0 insert <name> <file> [<type=usb|usb-ro|hdd|cdrom>]"
	echo "	   $0 eject <name>"
	echo "	   $0 mount <file> <mnt>"
	echo "	   $0 cleanup <file> <mnt>"
	echo "	   $0 ecm <name> <on|off>"
	exit 1
}

case "$1" in
	insert)
		shift
		usb_ms_insert "$@"
		;;
	eject)
		shift
		usb_ms_eject "$@"
		;;
	setup)
		shift
		setup_image "$@"
		;;
	mount)
		shift
		mount_image "$@"
		;;
	cleanup)
		shift
		cleanup_image "$@"
		;;
	ecm)
		shift
		ecm "$@"
		;;
	*)
		usage
		;;
esac
exit $?
