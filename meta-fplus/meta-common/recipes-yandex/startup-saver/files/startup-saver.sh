#!/bin/bash -e

set +e

STARTUP_DELAY=90
REBOOT_DELAY=60

while true; do
    ps ax | grep -v grep | grep "dbus-broker " >/dev/null || break
    sleep 1
done

sleep STARTUP_DELAY

# Check for some service that already must be here

