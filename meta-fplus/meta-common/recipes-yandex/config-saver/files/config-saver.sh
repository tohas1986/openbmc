#!/bin/sh

[ -f "/var/lock/config-saver.lock" ] && exit 0
touch /var/lock/config-saver.lock

source /usr/share/openrack/functions

echo Saving persistent configuration
persistent_save || :
sync
rm /var/lock/config-saver.lock
