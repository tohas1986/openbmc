#!/bin/sh

PATH=/sbin:/bin:/usr/bin:/usr/sbin

test -x /usr/sbin/ntpdate || exit 0

if test -f /etc/default/ntpdate ; then
. /etc/default/ntpdate
fi

if [ "$NTPSERVERS" = "" ] ; then
	if [ "$METHOD" = "" -a "$1" != "silent" ] ; then
		echo "Please set NTPSERVERS in /etc/default/ntpdate"
		exit 1
	else
		exit 0
	fi
fi

# This is a heuristic:  The idea is that if a static interface is brought
# up, that is a major event, and we can put in some extra effort to fix
# the system time.  Feel free to change this, especially if you regularly
# bring up new network interfaces.
if [ "$METHOD" = static ]; then
	OPTS="-b"
fi

if [ "$METHOD" = loopback ]; then
	exit 0
fi

(

# Wait for network to get configured
online=false
i=0
MAXTRIES=60
echo "Waiting for network to get configured"
while true
do
	networkctl status | grep -q "State: routable" && online=true && echo "Network is up, synching" && break
	i=$((i+1))
	[ $i -ge $MAXTRIES ] && echo "Timeout waiting for network, giving up" && exit
	sleep 1
done

LOCKFILE=/var/lock/ntpdate

# Avoid running more than one at a time
if [ -x /usr/bin/lockfile-create ]; then
	lockfile-create $LOCKFILE
	lockfile-touch $LOCKFILE &
	LOCKTOUCHPID="$!"
fi

if /usr/sbin/ntpdate -s $OPTS $NTPSERVERS 2>/dev/null; then
	if [ "$UPDATE_HWCLOCK" = "yes" ]; then
		hwclock --systohc || :
	fi
fi

if [ -x /usr/bin/lockfile-create ] ; then
	kill $LOCKTOUCHPID
	lockfile-remove $LOCKFILE
fi

) &

# wait for all subprocesses to finish
# this is required when using systemd service as ntpd will start before ntpdate finishes
# and results in a bind error (port 123)
wait
