#!/bin/bash

. /usr/share/openrack/functions

ENV_MAC_ADDR=`fw_printenv ethaddr`
MAC_ADDR=`echo $ENV_MAC_ADDR | cut -d "=" -f 2`
USBNAME="usbnet"
USBDEV="usb0"
ETHDEV="eth0"
VLANDEV="bpvlan"
BRDEV="bpbr"
FLAGFILE="/var/cache/private/usbnet.lock"
IGNOREFILE="/var/run/ignored_interfaces.txt"
VLAN=0
IP=192.168.255.254
NETMASK=24
CMD=""

usage() {
	echo "Usage: $0 <on/off/check> [VLAN ID] [IP address] [IP netmask in bits]"
	echo "          VLAN ID 0 means create local interface"
	exit 0
}

detect_dev() {
	ip link show $1 >/dev/null 2>&1
}

get_vlanid() {
	local vlan=$(ip link show | grep "$VLANDEV" | awk '{print $2}' | awk -F "." '{print $2}' | awk -F "@" '{print $1}')
	[ -z "$vlan" ] && echo ERR && return 0
	echo $vlan
}

usbnet_on() {
	# Bring device off if already exists
	local vlan=$(get_vlanid)
	detect_dev $VLANDEV.$vlan && detect_dev $USBDEV && detect_dev $BRDEV && usbnet_off

	local vlan=$1

	if [ ! -z $MAC_ADDR ]; then
		# Generate MAC Address from ethaddr using locally administered MAC
		# https://en.wikipedia.org/wiki/MAC_address#Universal_vs._local_(U/L_bit
		local SUBMAC=`echo $MAC_ADDR | cut -d ":" -f 2-6`
		usb-ctrl ecm $USBNAME on "06:$SUBMAC" "02:$SUBMAC"
	else
		usb-ctrl ecm $USBNAME on
	fi

	# Use NCM (Ethernet) Gadget instead of FunctionFS Gadget
	echo 0x0103 > /sys/kernel/config/usb_gadget/$USBNAME/idProduct
	echo "OpenBMC $USBNAME Device" > /sys/kernel/config/usb_gadget/$USBNAME/strings/0x409/product
	sleep 3

	ip link add name $BRDEV type bridge
	ip link set dev $BRDEV mtu 1440 up
	ip address flush $BRDEV
	ip link set dev $USBDEV master $BRDEV
	ip link set $USBDEV mtu 1440 up
	ip address flush $USBDEV
	if [ "$vlan" != "0" ]; then
		ip link add link $ETHDEV name $VLANDEV.$vlan type vlan id $vlan
		ip link set dev $VLANDEV.$vlan master $BRDEV
		ip link set $VLANDEV.$vlan mtu 1440 up
		ip address flush $VLANDEV.$vlan
	else
		ip address add $IP/$NETMASK dev $BRDEV
		iptables -F
		iptables -I INPUT -i $BRDEV -j DROP
		iptables -I INPUT -i $BRDEV -m tcp -p tcp --dport 443 -j ACCEPT
		iptables -I INPUT -i $BRDEV -p icmp -j ACCEPT
		iptables -I FORWARD -j DROP
	fi
	echo $vlan $IP $NETMASK > $FLAGFILE
	local _ignore="$USBDEV,$BRDEV"
	[ "$vlan" != "0" ] && _ignore="${_ignore},$VLANDEV.$vlan"
	echo "$_ignore" > $IGNOREFILE
	[ "$vlan" != "0" ] && lldp-forwarder $ETHDEV $USBDEV &
}

usbnet_off() {
	local vlan=$(get_vlanid)
	[ ! -d "/sys/kernel/config/usb_gadget/$USBNAME" -a "$vlan" = "ERR" ] && return 0
	killall lldp-forwarder || :

	detect_dev $VLANDEV.$vlan && ip link set $VLANDEV.$vlan down && ip link del name $VLANDEV.$vlan
	detect_dev $USBDEV && ip link set $USBDEV down && usb-ctrl ecm $USBNAME off
	detect_dev $BRDEV && ip link set dev $BRDEV down && ip link del name $BRDEV
	rm -f $FLAGFILE >/dev/null 2>&1 || :
	rm -f $IGNOREFILE >/dev/null 2>&1 || :
	iptables -F
	return 0
}

usbnet_check() {
	[ ! -f "$FLAGFILE" ] && echo usbnet: No flag file found, exiting && return 0
	local vlan=$(cat $FLAGFILE | awk '{print $1}')
	local ip=$(cat $FLAGFILE | awk '{print $2}')
	local netmask=$(cat $FLAGFILE | awk '{print $3}')
	[ "$ip" != "" ] && IP=$ip
	[ "$netmask" != "" ] && NETMASK=$netmask
	[ "$vlan" = "" ] && echo usbnet: Empty flag file found, removing and exiting && rm $FLAGFILE && return 0
	echo "usbnet: Starting usbnet with VLAN=$vlan (IP=$IP/$NETMASK)"
	usbnet_on $vlan
	return 0
}

############################## MAIN ##############################

# 1. Check if flag is present
# 2. If USBDEV exists and VLAN id in flag is the same - exit
# 3. Remove USBNAME, stop bridge and VLAN interface (unconditionally)
# 4. Create USBNET device, bridge, VLAN interface
case "$1" in
	"on") CMD=on ;;
	"off") CMD=off ;;
	"check") CMD=check ;;
	*) usage ;;
esac

case "$2" in
	''|*[!0-9]*) ;;
	*) VLAN="$2" ;;
esac

[ $VLAN -eq 0 -a "$3" != "" ] && IP="$3"
[ $VLAN -eq 0 -a "$4" != "" ] && NETMASK="$4"

[ "$CMD" = "on" ] && usbnet_on $VLAN
[ "$CMD" = "off" ] && usbnet_off
[ "$CMD" = "check" ] && usbnet_check
