#!/bin/sh

source /usr/share/openrack/functions

URL="https://hwwbus.haas.yandex-team.ru/api/v2/ipmi/MACMACMAC/password"


IF=$(networkctl | grep configured | grep eth | tail -1 | awk '{print $2}' | sed 's/eth//g')
MAC=$(getmac_if $IF | tr -d ":" | tr '[:upper:]' '[:lower:]')

URL=$(echo $URL | sed "s|MACMACMAC|$MAC|g")

curl_rc=$(curl --connect-timeout 5 --fail --silent -X POST -H 'Content-type: application/json' $URL || : 2>&1)
pass=$(echo $curl_rc | jq -M '.password' | tr -d "\"")
[ "$pass" != "" -a "$pass" != "null" ] && echo "ADMIN:$pass" | chpasswd && echo Password changed || echo Failed to change password
persistent_save || :
