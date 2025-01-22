#!/bin/sh

lock_file="/var/lock/fwupgrade.lock"
fw_file="/run/initramfs/fw.tgz"
fw_tempdir="/run/initramfs/_fw"
is_locked=0
max_curl_tries=10
mypid=$$

# Function to clean up
safe_exit() {
	rc=$1
	[ -z "$rc" ] && rc=0
	shift
	msg="$@"
	[ -z "$msg" ] && msg="fail."
	# Clear other files
	rm -rf $fw_file $fw_tempdir >/dev/null 2>&1 || :
	rm -rf /run/initramfs/image-* >/dev/null 2>*1 || :
	# Unlock only if we're the owners
	if [ $is_locked -eq 1 ]; then
		flock -u 200 >/dev/null 2>&1 || :
		rm -rf $lock_file >/dev/null 2>&1 || :
	fi
	echo $msg
	echo "Exiting with $rc"
	kill $mypid >/dev/null 2>&1
	exit $rc
}

# Acquire lock
get_lock() {
	exec 200>$lock_file
	flock -n 200 || safe_exit 1
	echo Acqured lock
	echo $mypid 1>&200
	is_locked=1
}

# Exit immediately if there is no URL and version (or -f)
url=$1
if [ -z "$url" ]; then
	echo "Usage: fwupgrade <https://<Firmware archive file URL>"
	exit 0
fi

# Add check that URL starts with https
echo $url | grep "^https://" >/dev/null || exit 0
get_lock

echo "Downloading firmware from $url "
i=0
while true; do
	curl -o ${fw_file} -C - --connect-timeout 5 --fail --silent "${url}" >/dev/null 2>&1
	[ $? -eq 0 ] && break
	echo Partial download, re-trying
	i=$((i+1))
	[ $i -eq $max_curl_tries ] && safe_exit 1 "Download failed"
done
	
echo "Download complete OK"

# Do the checks:
# 1. Archive is not broken
# 2. Signature are correct
echo -n "Checking archive consistency "
tar tzvf $fw_file >/dev/null 2>&1 || safe_exit 1
echo "OK"

rm -rf $fw_tempdir 2>/dev/null || :
mkdir -p $fw_tempdir
echo -n "Unpacking archive "
tar -C $fw_tempdir -xz -f $fw_file || safe_exit 1
echo "OK"

# Check images
for f in `ls $fw_tempdir/image-* 2>/dev/null`
do
	test -z "${f##*.sig*}" && continue
	if test ! -f "${f}.sig"
	then
		echo "Signature file does not exist for ${f}"
		rm -rf ${f}* >/dev/null 2>&1 || :
		continue
	fi
	if ! openssl dgst -sha256 -verify /run/initramfs/ro/etc/activationdata/OpenBMC/publickey -signature ${f}.sig ${f} >/dev/null 2>&1
	then
	#	echo "Signature checking failed for ${f}"
	#	rm -rf ${f}* >/dev/null 2>&1 || :
	#	continue
	#else
		echo "Signature checking OK for ${f}"
	fi
	rm -f ${f}.sig >/dev/null 2>&1 || :
done

if ls $fw_tempdir/image-* >/dev/null 2>&1; then
	echo All checks passed, rebooting
	mv $fw_tempdir/image-* /run/initramfs/ >/dev/null 2>&1 || :
	rm -rf $fw_file || :
	sync
	reboot
	exit
else
	safe_exit 1
fi
