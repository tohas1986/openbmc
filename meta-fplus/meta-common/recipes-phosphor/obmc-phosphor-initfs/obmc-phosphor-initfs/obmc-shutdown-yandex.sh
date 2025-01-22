#!/bin/sh

echo shutdown: "$@"

export PS1=shutdown-sh#

# exec bin/sh

cd /

# Mount debugfs (need for GPIO control)
[ ! -f /sys/kernel/debug/gpio ] && echo Remounting debugfs && mount debugfs /sys/kernel/debug -t debugfs || :

# Start led
source /oldroot/usr/share/openrack/functions || :
setfans || :
sled_green || :
sled_hb_fast || :

# Unset BMC_READY
_bmc_rdy 1 || :

# Disable LPC interrupts
# Same address/mask between AST2500 and AST2600
devmem 0x1e789080 32 0x401 || :

# Release BIOS
bios_release || :

# Check if there is archive
for f in `ls /*.tar 2>/dev/null`
do
	echo Archive found: $f
	tar xvf ${f} -C / >/dev/null 2>&1 || :
	rm -f ${f} >/dev/null 2>&1 || :
done

# Check images if any
export LD_LIBRARY_PATH=/oldroot/run/initramfs/ro/usr/lib
for f in `ls /image-* 2>/dev/null`
do
	test -z "${f##*.sig*}" && continue
	if test ! -f "${f}.sig"
	then
		echo "Signature file does not exist for ${f}"
		rm -rf ${f}* >/dev/null 2>&1 || :
		continue
	fi
	if ! /oldroot/run/initramfs/ro/usr/bin/openssl dgst -sha256 -verify /oldroot/run/initramfs/ro/etc/activationdata/OpenBMC/publickey -signature ${f}.sig ${f} >/dev/null 2>&1
	then
	#	echo "Signature checking failed for ${f}"
	#	rm -rf ${f}* >/dev/null 2>&1 || :
	#	continue
	#else
		echo "Signature checking OK for ${f}"
	fi
	rm -f ${f}.sig >/dev/null 2>&1 || :
done

# If there are update files - drop upgraded flag
if ls $image* > /dev/null 2>&1
then
	rm -f /oldroot/run/initramfs/rw/cow/etc/upgraded.flg >/dev/null 2>&1 || :
fi

# Unmount filesystems
if [ ! -e /proc/mounts ]
then
	mkdir -p /proc
	mount  proc /proc -tproc
	umount_proc=1
else
	umount_proc=
fi

# Remove an empty oldroot, that means we are not invoked from systemd-shutdown
rmdir /oldroot 2>/dev/null

# Move /oldroot/run to /mnt in case it has the underlying rofs loop mounted.
# Ordered before /oldroot the overlay is unmounted before the loop mount
mkdir -p /mnt
mount --move /oldroot/run /mnt

set -x
for f in $( awk '/oldroot|mnt/ { print $2 }' < /proc/mounts | sort -r )
do
	umount $f
done
set +x

update=/run/initramfs/update
image=/run/initramfs/image-
signed="no"
wdt="-t 1 -T 5"
wdrst="-T 15"

if ls $image* > /dev/null 2>&1
then
	if test -x $update
	then
		if test -c /dev/watchdog
		then
			echo Pinging watchdog ${wdt+with args $wdt}
			watchdog $wdt -F /dev/watchdog &
			wd=$!
		else
			wd=
		fi

		# Debug shell
		# exec /bin/sh

		$update --clean-saved-files --no-restore-files
		remaining=$(ls $image* 2>/dev/null)
		if test -n "$remaining"
		then
			echo 1>&2 "Flash update failed to flash these images:"
			echo 1>&2 "$remaining"
		else
			echo "Flash update completed."
		fi

		if test -n "$wd"
		then
			kill -9 $wd
			if test -n "$wdrst"
			then
				echo Resetting watchdog timeouts to $wdrst
				watchdog $wdrst -F /dev/watchdog &
				sleep 1
				# Kill the watchdog daemon, setting a timeout
				# for the remaining shutdown work
				kill -9 $!
			fi
		fi
	else
		echo 1>&2 "Flash update requested but $update program missing!"
	fi
fi

echo Remaining mounts:
cat /proc/mounts

test "$umount_proc" && umount /proc && rmdir /proc

# tcsattr(tty, TIOCDRAIN, mode) to drain tty messages to console
test -t 1 && stty cooked 0<&1

echo "Syncing..."
sync || :
sync || :
sync || :

wdt_reboot

# Execute the command systemd told us to ...
if test -d /oldroot  && test "$1"
then
	if test "$1" = kexec
	then
		$1 -f -e
	else
		$1 -f
	fi
fi

echo "Execute ${1-reboot} -f if all unmounted ok, or exec /init"

export PS1=shutdown-sh#\ || :
exec /bin/sh
