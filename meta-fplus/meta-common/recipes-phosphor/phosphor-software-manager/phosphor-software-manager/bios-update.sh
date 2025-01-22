#!/bin/bash

# 0. Check input file (TBD - signature check)
# 1. Check host power
# 2. Do power soft
# 3. Wait for host to get offline
# 4. Take BIOS via GPIO
# 5. Instantiate SPI flash, find corresponding MTD
# 6. Check flash is there
# 7. Flash the image
# 8. De-instantiate SPI flash
# 9. Release BIOS via GPIO
# 10. Power host on

SPI_DEV="1e630000.spi"
SPI_PATH="/sys/bus/platform/drivers/spi-aspeed-smc"

# The file which represent the lock.
LOCKFILE="`basename $0`.lock"

# Create the lockfile.
touch $LOCKFILE

# Create a file descriptor over the given lockfile.
echo "Creating Lock file"
exec {FD}<>$LOCKFILE

if ! flock -n $FD; then
	echo "Another instance of `basename $0` is running."
	exit 1
fi

usage()
{
	echo "Usage: bios-updater.sh <BIOS image directory>"
	return 0
}

IMAGE_DIR=$1

rm_image()
{
	rm -rf $IMAGE_DIR >/dev/null 2>&1 || :
	return 0
}

[ ! -s "$IMAGE_DIR/MANIFEST" ] && usage && exit 0

source $1/MANIFEST || :

IMAGE_FILE="$1/$ImageFile"
[ ! -s "$IMAGE_FILE" ] && echo "No image file $IMAGE_FILE" && usage && exit 1

results=$(openssl dgst -sha256 -verify /etc/activationdata/OpenBMC/publickey -signature ${IMAGE_FILE}.sig $IMAGE_FILE)
[ "$results" = "Verification Failure" ] && echo "Verification Failure" && rm_image && exit 1

source /usr/share/openrack/functions

host_power_status="off"
power_status && host_power_status="on" && echo "Powering host down"

# Check host power and switch power off
i=0
while true; do
	power_status || break
	ipmitool power off >/dev/null 2>&1
	sleep 10
	i=$((i+1))
	[ $i -eq 10 ] && echo "Unable to power off the server, exiting" && rm_image && exit 1
done

echo "Host powered off"

# If platfrom based on Intel CPU, put ME to recovery mode
mcinfo=$(mktemp)
ipmitool mc info  > $mcinfo 2>&1
[ $? -ne 0 ] && echo "$bmc not responding to IPMI" && rm_image && exit 1

bmc_vid=$(cat $mcinfo | grep "Manufacturer ID" | awk '{print $4}')
bmc_pid=$(cat $mcinfo | grep "Product ID" | awk '{print $4}')

if [ "$host_cpu" == "intel" ]; then
	echo "ME to recovery mode"
	me_ipmb $ME_CMD_RECOVER
	sleep 10
fi

# Instantiate BIOS flash
bios_insert

# Check flash is detected
mtd=$(bios_get_mtd)
[ -z "$mtd" ] && bios_release && echo "MTD device not detected, exiting" && rm_image && exit 1

# Enable reboot guard
echo "Disabling reboot"
systemctl start reboot-guard-enable

# Flash the BIOS
echo "Bios upgrade started"
flashcp -v $IMAGE_FILE /dev/$mtd >/dev/null 2>&1
if [ $? -eq 0 ]; then
		echo "BIOS update sucessfull"
	else
		echo "BIOS update failed"
fi

# Enable reboot guard
echo "Enabling reboot"
systemctl start reboot-guard-disable

# Unbind flash driver
bios_eject

#If platfrom based on Intel CPU, resetting ME after BIOS update
if [ "$host_cpu" == "intel" ]; then
	echo "ME resetting"
	me_ipmb $ME_CMD_RESET
	sleep 10
fi

# Restore power
[ "$host_power_status" = "on" ] && echo "Powering host up" && ipmitool power on >/dev/null 2>&1

rm_image

echo "Deleting lock file"
exec {FD}>&-
rm -rf $LOCKFILE
