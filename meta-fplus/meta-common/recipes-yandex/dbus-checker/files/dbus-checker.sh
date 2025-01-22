#!/bin/sh
# FIXME
exit 0

# Wait for 120 seconds of uptime
while true; do
	uptime=$(cat /proc/uptime | awk -F "." '{print $1}')
	[ $uptime -gt 60 ] && break
	sleep 1
done

# Check for dbus-broker block
systemctl status >/dev/null 2>&1 &
check_pid=$!
int=0
logger "Checking dbus status"
while true; do
	ps ax | grep -v grep | grep $check_pid >/dev/null 2>&1 && i=$((i+1)) || i=0
	if [ $i -gt 30 ]; then
		logger "D-Bus lock detected, rebooting"
		reboot &
		killall dbus-broker || :
	fi
	[ $i -eq 0 ] && break
	sleep 1
done
