#!/bin/sh

source /usr/share/openrack/functions
source /usr/sbin/bmc-self-test-functions

[ "$host_cpu" = "none" ] && exit 0

echo BMC self test service running

clear_old_logs || echo "ERROR - Failed to clear old logs"
add_start
get_date || echo "ERROR - Failed to get date"
get_ipv6 || echo "ERROR - Failed to get IPv6"
get_uptime || echo "ERROR - Failed to get uptime"
get_hwwbus_status || echo "ERROR - Failed to get HWWbus status"
check_fans || echo "ERROR - Failed to check fans"
check_services || echo "ERROR - Failed to check services"
toggle_led || echo "ERROR - Failed to toggle leds"
add_end

