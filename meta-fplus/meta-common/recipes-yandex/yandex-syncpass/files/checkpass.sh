#!/bin/sh

source /usr/share/openrack/functions

ro_pass=$(cat /run/initramfs/ro/etc/shadow | grep ^ADMIN | awk -F ":" '{print $2}' | tr -d '$')
rw_pass=$(cat /etc/shadow | grep ^ADMIN | awk -F ":" '{print $2}' | tr -d '$')

[ "$ro_pass" == "$rw_pass" ] && echo checkpass: Need to sync password && systemctl start --no-block syncpass.service && exit 0
echo checkpass: No need to sync password
