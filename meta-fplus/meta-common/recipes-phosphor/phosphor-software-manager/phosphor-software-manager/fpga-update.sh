#!/bin/bash

# 0. Check input file (TBD - signature check)
# 1. Stop GPU sensor
# 2. Check flash is there
# 3. Flash the image

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
	echo "Usage: fpga-updater.sh <FPGA image directory>"
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

bmc_vid=$(cat $mcinfo | grep "Manufacturer ID" | awk '{print $4}')
bmc_pid=$(cat $mcinfo | grep "Product ID" | awk '{print $4}')

# Enable reboot guard
echo "Disabling reboot"
systemctl start reboot-guard-enable

# Stop GPU Sensor service
echo "Stopping GPU sensor service"
systemctl stop dobby
systemctl stop xyz.openbmc_project.gpusensor.service

# Flash FPGA
echo "FPGA upgrade started"
/usr/sbin/nvidia-fpga-updater -p $IMAGE_FILE
if [ $? -eq 0 ]; then
	echo "FPGA update sucessfull"
 else
	echo "FPGA update failed"
fi

# Enable reboot guard
echo "Enabling reboot"
systemctl start reboot-guard-disable

# Start GPU Sensor service
echo "Starting GPU sensor service"
systemctl start xyz.openbmc_project.gpusensor.service
systemctl start dobby
rm_image

echo "Deleting lock file"
exec {FD}>&-
rm -rf $LOCKFILE

echo "Please do Power Cycle from connected Node side directly"

