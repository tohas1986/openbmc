#!/bin/sh

urls="https://hwwbus.haas.yandex-team.ru/api/v2/ipmi/ https://hwwbus.pt.cloud.yandex-team.ru/api/v2/ipmi/ https://hwwbus.gpn.cloud.yandex-team.ru/api/v2/ipmi/"

lock_file="/var/lock/fwpoll.lock"
json_file=""
is_locked=0
mypid=$$

# Function to clean up
safe_exit() {
	rc=$1
	[ -z "$rc" ] && rc=0
	shift
	msg="$@"
	# Clear other files
	rm -rf $json_file $parsed >/dev/null 2>&1 || :
	# Unlock only if we're the owners
	if [ $is_locked -eq 1 ]; then
		flock -u 200 >/dev/null 2>&1 || :
		rm -rf $lock_file >/dev/null 2>&1 || :
	fi
	[ -n "$msg" ] && echo $msg
	kill $mypid >/dev/null 2>&1
	exit
}

# Acquire lock
get_lock() {
	exec 200>$lock_file
	flock -n 200 || safe_exit 1
	echo $mypid 1>&200
	is_locked=1
}

source /usr/share/openrack/functions

ipmi_mac=$(getmac_if 0 | tr '[:upper:]' '[:lower:]')

get_lock

tmp=$(mktemp)

json_file=""
for u in $urls; do
	url="${u}/bmc/${ipmi_mac}/firmware"
	_msg="Trying $url :"
	curl -o ${tmp} --connect-timeout 5 --fail --silent "${url}" >/dev/null 2>&1
	[ -s "$tmp" ] && echo $_msg OK && json_file=$tmp && break
	echo $_msg FAIL
done

[ ! -n "$json_file" ] && rm -rf $tmp && safe_exit 0

parsed=$(mktemp)
cat $json_file | /usr/share/openrack/JSON.sh -l -p -n -b | tr -d '[' | tr -d ']' | tr -d '"' > $parsed
[ ! -s "$parsed" ] && rm -rf $parsed && safe_exit 0

my_ver=$(cat /etc/os-release  | grep VERSION= | sed -e 's/VERSION=//g' | tr -d "\"")

new_ver=$(cat $parsed | grep "firmware,version" | awk '{print $2}')
fw_url=$(cat $parsed | grep "firmware,URL" | awk '{print $2}')
action=$(cat $parsed | grep "firmware,action" | awk '{print $2}')

[ -z "$my_ver" -o -z "$new_ver" -o -z "$action" -o -z "$fw_url" ] && safe_exit 0

[ "$action" != "upgrade" ] && safe_exit 0

[ "$my_ver" = "$new_ver" ] && safe_exit 0

echo Upgrading from $my_ver to $new_ver using $fw_url
/usr/sbin/fwupgrade $fw_url
