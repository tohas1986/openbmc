#!/bin/sh

[ "$1" != "on" -a "$1" != "off" ] && exit 0

source /usr/share/openrack/functions

[ "$host_cpu" = "none" ] && exit 0

rdy=0
[ "$1" = "off" ] && rdy=1

gpioset $(gpiofind BMC_INIT)=1 >/dev/null 2>&1 && echo Setting BMC_INIT to 1 || echo Unable to set BMC_INIT

echo Setting BMC_READY to $rdy
_bmc_rdy $rdy

if [ $1 = "on" ]; then
    echo Enabling snoop IRQ
    snoop_irq_en
    sleep 1
    diff=$(snoop_irq_rate)
    [ $diff -gt 3000 ] && echo "IRQ storm detected, disabling snoop IRQ" && snoop_irq_dis || :
else
    echo Disabling snoop IRQ
    snoop_irq_dis
fi
