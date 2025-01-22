#!/bin/sh

source /usr/share/openrack/functions

echo "Updating motd"
ipv6=$(get_ipv6)
[ "$ipv6" != "" ] && cat /run/initramfs/ro/etc/motd | sed "s/IPv6.*/IPv6: $ipv6/g" > /etc/motd || :
