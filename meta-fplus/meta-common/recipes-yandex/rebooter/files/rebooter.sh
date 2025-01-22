#!/bin/sh

source /usr/share/openrack/functions

MEMLIMIT=150000
FREELIMIT=50
DAYSLIMIT=10
RACKLAN_MAX_FAIL=10
ETH1_MAX_FAIL=10
PULLER_POLL=20

sum() {
   sum=0
   while read -r line ; do
       sum=$((sum+line))
   done
   echo $sum
}

rmc_fail=0
eth1_fail=0
puller_cnt=0
final_countdown=$((RANDOM%100 + 10))
while true; do
	# Memory consumption
	mem_usage=$(top -n 1 | grep -E 'lua|nginx' | grep -v grep | awk '{print $5}' | sum)
	echo $mem_usage > /run/board_memusage
	[ -n "$mem_usage" ] && [ $mem_usage -gt $MEMLIMIT ] && /sbin/reboot

	memfree=$(free -m | grep "Mem:" | awk '{print $4}')
	[ -n "$memfree" ] &&  [ $memfree -lt $FREELIMIT ] && /sbin/reboot

	# Maximum uptime
	daysup=$(uptime | grep days | awk '{print $3}')
	if [ -n "$daysup" ] && [ $daysup -gt $DAYSLIMIT ]; then
		final_countdown=$((final_countdown-1))
		[ $final_countdown -eq 0 ] && /sbin/reboot
	fi

    # IPv6 on interface

	# Master RMC jobs
	if am_i_master; then
		# Check I2C-0 status
		check_bus || :

		# Puller call
		puller_cnt=$((puller_cnt+1))
		if [ $puller_cnt -eq $PULLER_POLL ]; then
			puller_cnt=0
			/usr/sbin/puller || :
		fi
	fi

	# Check fan control
	if [ "$t" = "CB" ]; then
		systemctl status ya-pid5 >/dev/null 2>&1
		[ $? -eq 3 ] && systemctl restart ya-pid5

		# Check eth1 got global IPv6 address
		ifconfig eth1 | grep "inet6 addr" | grep "Scope:Global" >/dev/null 2>&1 || eth1_fail=$((eth1_fail+1))

		if [ $eth1_fail -gt $ETH1_MAX_FAIL ]; then
			echo eth1 has no address, reseting switch
			# Need to reset switch and then ifup/ifdown eth1
			# racklan interface will be up later
			reset-b53
			ifconfig eth1 down > /dev/null 2>&1
			ifconfig eth1 up > /dev/null 2>&1
			eth1_fail=0
		fi

		[ $eth1_fail -eq 0 ] || echo ETH1_FAIL: $eth1_fail

		# Check rack VLAN availability (only if eth1 is OK)
		ping6 -I racklan -c 1 rmc-a >/dev/null 2>&1 && rmc_fail=0 || \
			ping6 -I racklan -c 1 rmc-b >/dev/null 2>&1 && rmc_fail=0 || \
			[ $eth1_fail != 0 ] && rmc_fail=0 || \
			rmc_fail=$((rmc_fail+1))

		[ $rmc_fail -eq 0 ] || echo RMC_FAIL: $rmc_fail

		if [ $rmc_fail -gt $RACKLAN_MAX_FAIL ]; then
			echo Unable to ping RMC for $RACKLAN_MAX_FAIL cycles, reselecting rack prefix
			rm -rf /etc/rack_prefix >/dev/null 2>&1 || :
			/usr/share/openrack/genprefix
			rmc_fail=0
		fi
	fi

	# Enable racklan in case systemd-networkd screwed up
	ifconfig racklan up >/dev/null 2>&1 || :

	sleep 30
done
