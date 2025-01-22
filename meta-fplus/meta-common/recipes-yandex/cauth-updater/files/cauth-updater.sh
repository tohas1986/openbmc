#!/bin/bash

log() {
	logger -t "CAuth-updater" "$@"
}

cauth_key="svc_rmmsupport"
cauth_urls="https://hwwbus.pt.cloud.yandex-team.ru/cauth/userkeys https://hwwbus.gpn.cloud.yandex-team.ru/cauth/userkeys https://cauth.yandex.net:4443/userkeys"
curl_cmd="curl -s --connect-timeout 1 --retry 1"

ssh_dir="/home/root/.ssh"
key_file="$ssh_dir/authorized_keys"
magic_ident="emergency@vault"

[ -d $ssh_dir ] || (mkdir $ssh_dir; chmod 0700 $ssh_dir;)
[ -f $key_file ] || (touch $key_file; chmod 0600 $key_file;)

emergency_key=$(cat $key_file | grep $magic_ident)

# Wait for the network
echo "Waiting for network"
i=5
while true; do
	ip -6 r s | grep default >/dev/null 2>&1 && echo "IPv6 Network acquired" && break
	ip -4 r s | grep default >/dev/null 2>&1 && echo "IPv4 Network acquired" && break
	i=$((i-1))
	[ $i -eq 0 ] && echo "No network, exiting" && exit
	sleep 2
done

tmp1=$(mktemp)
for cauth_url in $cauth_urls; do
	echo -n "Trying $cauth_url :"
	$curl_cmd ${cauth_url}/${cauth_key} > $tmp1 2>/dev/null
	[ $? == 0 -a -s "$tmp1" ] && echo OK && break
	echo FAIL
done

if [ ! -s "$tmp1" ]; then
	echo Empty response
	cat $tmp1
	rm -rf $tmp1
	exit 0
fi

tmp2=$(mktemp)
cat $tmp1  | grep -v "#####" | awk '{print $3, $4, $5}' > $tmp2

rm -rf $tmp1 >/dev/null 2>&1

if [ ! -s $tmp2 ]; then
	log "Empty response"
	rm -rf $tmp2
	exit 0
fi

md5_new=$(cat $tmp2 | md5sum)
md5_old=$(cat $key_file | grep -v $magic_ident | md5sum)

if [ "$md5_new" != "$md5_old" ]; then
	log "Updating keys file"
	echo $emergency_key >> $tmp2
	mv $tmp2 $key_file
	if ! grep -s "\-g" /etc/default/dropbear; then
		echo "Disabling password auth"
		echo 'DROPBEAR_EXTRA_ARGS="-B -g"' > /etc/default/dropbear
		systemctl --no-block restart dropbear
	fi
fi
rm -rf $tmp2 > /dev/null 2>&1
