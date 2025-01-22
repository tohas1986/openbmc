#!/bin/sh

# Import machine settings
mode="os"
[ -s "/oldroot/usr/share/openrack/machine" ] && source /oldroot/usr/share/openrack/machine && mode="shutdown"
[ -s "/usr/share/openrack/machine" ] && source /usr/share/openrack/machine

#
# Low-level functions (GPIO, I2C)
#

SOC_GPIO_BASE=$(cat /sys/devices/platform/ahb/ahb\:apb/1e780000.gpio/gpio/gpiochip*/base)

# GPIO definition
BMC_READY="BMC_READY"
BIOS_SEL="BIOS_SEL"

FRU_PATH="/sys/bus/i2c/devices/${fru_bus}-00${fru_addr}/eeprom"

PATH=$PATH:/bin:/usr/bin:/usr/local/bin:/sbin:/usr/sbin:/usr/local/sbin

# Get Aspeed silicon revision (G5, G6)
# Issue here is that SCU register containing SiliconID is
# different between G5 and G6
# G5 - 0x1e6e207c
# G6 - 0x1e6e2004
# That's why it's better to have helper function
_ast_scu2rev()
{
	local val="$1"
	[ -z "$val" ] && echo UNKNOWN && return 0

	case $val in
		0x05000303)
			echo AST2600-A0
			;;
		0x05010303)
			echo AST2600-A1
			;;
		0x05010303)
			echo AST2600-A2
			;;
		0x05030303)
			echo AST2600-A3
			;;
		0x05010203)
			echo AST2620-A1
			;;
		0x05010203)
			echo AST2620-A2
			;;
		0x05030203)
			echo AST2620-A3
			;;
		0x04000303)
			echo AST2500-A0
			;;
		0x04000103)
			echo AST2510-A0
			;;
		0x04000203)
			echo AST2520-A0
			;;
		0x04000403)
			echo AST2530-A0
			;;
		0x04010303)
			echo AST2500-A1
			;;
		0x04010103)
			echo AST2510-A1
			;;
		0x04010203)
			echo AST2520-A1
			;;
		0x04010403)
			echo AST2530-A1
			;;
		0x04030303)
			echo AST2500-A2
			;;
		0x04030103)
			echo AST2510-A2
			;;
		0x04030203)
			echo AST2520-A2
			;;
		0x04030403)
			echo AST2530-A2
			;;
		*)
			echo "UNKNOWN"
			;;
	esac
	return 0

}

ast_getrev()
{
	local val=$(devmem 0x1e6e207c)
	local socname=$(_ast_scu2rev $val)
	[ -z "$socname" -o "$socname" = "UNKNOWN" ] && val=$(devmem 0x1e6e2004) && socname=$(_ast_scu2rev $val)
	[ -z "$socname" -o "$socname" = "UNKNOWN" ] && echo UNKNOWN
	echo $socname | grep AST25 >/dev/null 2>&1 && echo G5
	echo $socname | grep AST26 >/dev/null 2>&1 && echo G6
	return 0
}

# Calculate GPIO number
# Accepts: [0-9]*, [0-9].[0-9]*, [A-Z].[0-9]*
gpio_n()
{
	local base="$1"
	local n="$2"
	# the X.Y pin passed
	if [ "${n%%.*}" != "$n" ]; then
			local port="${n%%.*}"
			local pin="${n##*.}"
		case "$port" in
		[0-9]*) ;;
		[A-Z])
			# Calculate offset A=65
			port=$(($(printf "%d" "'$port")-65))
			base=$SOC_GPIO_BASE
			;;
		esac
		n="$((port * 8 + pin))"
	fi
	echo -n "$((base + n))"
}

# Get GPIO number by name
gpio_name2n()
{
	cat /sys/kernel/debug/gpio | grep $1 | awk '{print $1}' | sed 's/gpio-//g'
}

# Find GPIO name by number
gpio_n2name()
{
	cat /sys/kernel/debug/gpio | grep gpio-$1 | awk '{print $2}' | tr -d "(" | tr -d ")"
}

# Enable GPIO
# $1 - line name
gpio_en()
{
	local g="$(gpio_name2n $1)"
	[ ! -e "/sys/class/gpio/gpio$g" ] || return 0
	echo "$g" > /sys/class/gpio/export
}

# Disable GPIO
gpio_del()
{
	local g="$(gpio_name2n $1)"
	[ -e "/sys/class/gpio/gpio$g" ] || return 0
	echo "$g" > /sys/class/gpio/unexport
}

# Get GPIO value
gpio()
{
	[ -z "$2" ] || { gpio_out $@; return $?; }
	local gpio="/sys/class/gpio/gpio$(gpio_name2n $1)"
	[ -e "$gpio" ]
	echo in > "$gpio/direction"
	cat "$gpio/value"
}

# $1 - line name
# $2 - value to write (1 - hi, 0 - lo)
gpio_out()
{
	[ -n "$2" ] && [ "$2" = 0 -o "$2" = 1 ]
	gpioset $(gpiofind $1)=$2 || :
}

# Add i2c device instance
i2c_en()
{
	local bus="$1"
	local slave="$(printf "%04x" $2)"
	local drv="$3"
	[ ! -e "/sys/bus/i2c/devices/$bus-$slave" ] || return 0
	echo "$drv" "0x$slave" > "/sys/bus/i2c/devices/i2c-$bus/new_device"
}

i2c_del()
{
	local bus="$1"
	local slave="$(printf "%04x" $2)"
	[ -e "/sys/bus/i2c/devices/$bus-$slave" ] || return 0
	echo "0x$slave" > "/sys/bus/i2c/devices/i2c-$bus/delete_device"
}

# Select/deselect a channel
i2c_mux()
{
	local bus="$1"
	local slave="$(printf "0x%02x" $2)"
	local chan="$3"
	if [ "$3" -lt 0 -o "$3" -gt 7 ]; then
		# Deselect
		chan=0
	else
		chan=$((1 << $3))
	fi
	i2cset -y "$bus" "$slave" 0 "$chan"
}

# Deselect all channels
i2c_demux()
{
	i2c_mux "$1" "$2" -1
}

# Do i2cdetect in a fashion way
i2c_scan()
{
	# TODO: check the i2cdetect exit code
	i2cdetect -y "$1" | sed 's, [0-9a-f]\{2\}, UU,g'
}

# Request GPIO base
io_chip()
{
	local base=0
	local bus="$1"
	local slave="$(printf "%04x" $2)"
	for b in /sys/class/gpio/gpiochip*; do
		local path="$(readlink -f $b)"
		local chip="${b##*/}"
		[ "$path" != "${path%%/$bus-$slave/gpio/$chip}" ] || continue
		base="${chip##gpiochip}"
		break
	done
	echo -n "$base"
}

# Enable IO expander, optionally request for one GPIO pin
io_en()
{
	local bus="$1"
	local slave="$2"
	i2c_en "$bus" "$slave" pca9675
	[ -n "$3" ] || return 0
	io_gpio_en "$bus" "$slave" "$3"
}

# Disalbe IO expander
io_del()
{
	i2c_del "$1" "$2"
}

# Enable specific GPIO on IO expander
io_gpio_en()
{
	local io="$(io_chip "$1" "$2")"
	gpio_en "$io" "$3"
}

# Get/set IO expander GPIO value
io_gpio()
{
	local n="$3"
	local io="$(io_chip "$1" "$2")"
	gpio "$io" "$3" $4
}

# Get MAC address of the interface $1 is the iface number
getmac_if()
{
	local a="eth"$1
	local mac_if=$(ifconfig $a 2>/dev/null| awk -v iface="$a" '$0 ~ iface {print toupper($5)}')
	[ -n "$mac_if" ] || return -1
	echo $mac_if
}

# Get MAC address from u-boot inveronment. $1 is iface number (0 or 1)
getmac_env()
{
	local a="eth"$1"addr"
	a=$(echo $a | tr -d "0")
	local mac_if=$(fw_printenv | awk -v iface="$a" -F "=" '$0 ~ iface {print toupper($2)}')
	[ -n "$mac_if" ] || return -1
	echo $mac_if
}

# Get serial number from U-Boot environment
getserial_env()
{
	local serial=$(fw_printenv | awk -F "=" '/serialnumber/ {print $2}')
	[ -z "$serial" ] && return -1
	echo $serial
}

get_fw_env_var() {
	/sbin/fw_printenv | grep $1 |  sed -ne '/^$/,$d' -e "s/^$1=//p"
}

dump_eeprom() {
	strings ${FRU_PATH}
}

mtdname2dev()
{
	[ -z "$1" ] && return -1
	local dev=$(grep "\"${1}\"" /proc/mtd | awk -F ":" '{print $1}') || return -1
	echo /dev/$dev
}

mtdname2block()
{
	[ -z "$1" ] && return -1
	local dev=$(grep "\"${1}\"" /proc/mtd | awk -F ":" '{print $1}' | sed 's/mtd/mtdblock/g' ) || return -1
	echo /dev/$dev
}

getmtdsum()
{
	[ -z "$1" ] && return -1
	local dev=$(mtdname2dev $1) || echo ERROR
	[ -e "$dev" ] || echo ERROR
	cat $dev | sha256sum | awk '{print $1}'
}

print_scu() {
	[ ! -e "/dev/mem" ] && mknod -m 660 /dev/mem c 1 1 && chown root:kmem /dev/mem
	local i
	local val
	for i in 04 0c 84 8c 90 94 a4 a8; do
		val=$(devmem 0x1e6e20$i)
		echo SCU$i $val
	done
}

_bmc_rdy() {
	[ "$mode" = "shutdown" -a "$host_cpu" = "none" ] && return 0
	[ "$1" != "0" -a "$1" != "1" ] && return 0
	# BMC Ready
	gpioset $(gpiofind $BMC_READY)=$1
}

# LEDs control
# usage:
#	  _sled_out <delay between write to trigger> <green led trigger> <red led trigger>
#	  trigger can be "timer", "none", "default-on"
_sled_out()
{
	[ -z "$1" -o -z "$2" -o -z "$3" ] && return -1
	local delay=$1
	local grn=/sys/class/leds/status_green
	local red=/sys/class/leds/status_amber
	echo $2 > $grn/trigger
	sleep $delay
	echo $3 > $red/trigger
}

# Steady green
sled_on_green()
{
	_sled_out 0 default-on none
}

# Steady red
sled_on_red()
{
	_sled_out 0 none default-on
}

# Steady yellow
sled_on_yellow()
{
	_sled_out 0 default-on default-on
}

# Off
sled_off()
{
	_sled_out 0 none none
}

# Blinking green
sled_green()
{
	_sled_out 0 timer none
}

# Blinking red
sled_red()
{
	_sled_out 0 none timer
}

# Blinking yellow
sled_yellow()
{
	_sled_out 0 timer timer
}

# Off-green-yellow-red
sled_xmas()
{
	_sled_out 0.3 timer timer
}

# Red-green
sled_trafficlight()
{
	_sled_out 0.5 timer timer
}

# Heartbeat manipulation functions
sled_hb_slow()
{
	echo timer > /sys/class/leds/heartbeat/trigger || :
	echo 1000 > /sys/class/leds/heartbeat/delay_on || :
	echo 1000 > /sys/class/leds/heartbeat/delay_off || :
}

sled_hb_norm()
{
	echo timer > /sys/class/leds/heartbeat/trigger || :
	echo 500 > /sys/class/leds/heartbeat/delay_on || :
	echo 500 > /sys/class/leds/heartbeat/delay_off || :
}

sled_hb_fast()
{
	echo timer > /sys/class/leds/heartbeat/trigger || :
	echo 100 > /sys/class/leds/heartbeat/delay_on || :
	echo 100 > /sys/class/leds/heartbeat/delay_off || :
}

sled_hb_hb()
{
	echo heartbeat > /sys/class/leds/heartbeat/trigger || :
}

# Compare two files and return TRUE if they diff
is_files_diff()
{
	local f1="$1"
	local f2="$2"
	# FIXME Not sure the following line is 100% correct
	[ ! -f "$f1" -o ! -f "$f2" ] && return 0
	local md5_1=$(cat $f1 | md5sum | awk '{print $1}')
	local md5_2=$(cat $f2 | md5sum | awk '{print $1}')
	[ "$md5_1" != "$md5_2" ] && return 0
	return 1
}

# Same but opposite
is_files_equal()
{
	is_files_diff $1 $2
	return $((1-$?))
}

# Return TRUE if EEPROM is filled with AMI FRU
fru_is_ami()
{
	dd if=$FRU_PATH bs=1 count=16 2>/dev/null | strings -n 1 | grep GIGABYTE >/dev/null 2>&1
}

# Fetch XML from AMI EEPROM and save it to /etc/fru/fru.xml
fru_get_xml()
{
	local tmp=$(mktemp)
	dd if=$FRU_PATH bs=1 skip=$((0x200)) count=8192 2>/dev/null | gzip -dc > $tmp 2>/dev/null
	is_files_diff $tmp /etc/fru/fru.xml && mv $tmp /etc/fru/fru.xml
	rm -rf $tmp >/dev/null 2>&1 || :
}

# Get FRU from EEPROM and echo name of the file with it
fru_get_header()
{
	local tmp=$(mktemp)
	dd if=$FRU_PATH of=$tmp bs=1 count=511 2>/dev/null
	echo $tmp
}

# Save FRU (EEPROM header) at 0x4000 of EEPROM
# Backup EEPROM to /etc/fru/eeprom_backup.bin
fru_backup_header()
{
	local header=$(fru_get_header)
	dd if=$FRU_PATH of=/etc/fru/eeprom_backup.bin
	dd if=$header of=$FRU_PATH bs=1 skip=$((0x4000)) >/dev/null 2>&1
	rm -rf $header >/dev/null 2>&1
}

# Generate FRU from XML file located at /etc/fru/fru.xml
fru_generate_header()
{
	[ -s "/etc/fru/fru.xml" ] || return 1
	local tmp=$(mktemp)
	/usr/bin/mkfru /etc/fru/fru.xml $tmp
	is_files_diff $tmp /etc/fru/yandex.fru.bin && mv $tmp /etc/fru/yandex.fru.bin
}

# Return true if two files are different
is_file_diff()
{
	diff $1 $2 >/dev/null 2>&1
	return $((1-$?))
}

# Write /etc/fru/yandex.fru.bin to EEPROM if needed; backup AMI header
fru_write_to_eeprom()
{
	[ ! -s "/etc/fru/yandex.fru.bin" ] && return 1
	local old_fru=$(fru_get_header)
	# If generated FRU header differs from what is in EEPROM - backup EEPROM and re-write FRU in the IC
	is_file_diff /etc/fru/yandex.fru.bin $old_fru && fru_backup_header && dd if=/etc/fru/yandex.fru.bin of=$FRU_PATH bs=1 >/dev/null 2>&1
	rm -rf $old_fru >/dev/null 2>&1
}

# Power off server using busctl
power_off() {
	echo "Shutting down Server"
	busctl set-property xyz.openbmc_project.State.Chassis /xyz/openbmc_project/state/chassis0 xyz.openbmc_project.State.Chassis RequestedPowerTransition s xyz.openbmc_project.State.Chassis.Transition.Off
}

# Power on server using busctl
power_on() {
	echo "Powering on Server"
	busctl set-property xyz.openbmc_project.State.Chassis /xyz/openbmc_project/state/chassis0 xyz.openbmc_project.State.Chassis RequestedPowerTransition s xyz.openbmc_project.State.Chassis.Transition.On
}

# Power status server using busctl
power_status() {
	ps ax | grep -v grep | grep power-control >/dev/null
	if [ $? -eq 0 ]; then
		ipmitool power status | grep "Power is off" >/dev/null && return 1
	else
		gpioget $(gpiofind PS_PWROK) 2>&1 | grep "0" >/dev/null 2>&1 && return 1
	fi
	return 0
}

# Power reset server using busctl
power_reset() {
	echo "Reset on server"
	busctl set-property xyz.openbmc_project.State.Chassis /xyz/openbmc_project/state/chassis0 xyz.openbmc_project.State.Chassis RequestedPowerTransition s xyz.openbmc_project.State.Chassis.Transition.Reset
}

# Memory note regarding P2A
# To disable flash access, bit#22 need to be set to 1
# To disable SoC register access, bit #23 need to be set to 1
# To disable LPC bridge, bit #24 need to be set to 0
# Currently only block flash access
p2a_unlock() {
	local socver=$(ast_getrev)
	local addr=0x1e6e202c
	[ "$socver" = "G6" ] && addr="0x1e6e20c0"
	local scu=$(devmem $addr)
	scu=$(($scu & 0xffbfffff))
	devmem $addr 32 $scu
}

p2a_lock() {
	local socver=$(ast_getrev)
	local addr=0x1e6e202c
	[ "$socver" = "G6" ] && addr="0x1e6e20c0"
	local scu=$(devmem $addr)
	scu=$(($scu | 0x400000))
	devmem $addr 32 $scu
}

p2a_status() {
	local socver=$(ast_getrev)
	local addr=0x1e6e202c
	[ "$socver" = "G6" ] && addr="0x1e6e20c0"
	local scu=$(devmem $addr)
	local flash=$(($scu & 0x400000))
	local regs=$(($scu & 0x800000))
	local lpc=$(($scu & 0x1000000))
	[ "$flash" = "0" ] && flash=unlocked || flash=locked
	[ "$regs" = "0" ] && regs=unlocked || regs=locked
	[ "$lpc" = "0" ] && lpc=unlocked || lpc=locked
	echo FLASH:$flash REGS:$regs LPC:$lpc
}

gpio_name_get() {
	gpioget $(gpiofind $1)
}

get_smbios() {
	[ -s "/var/lib/smbios/smbios2" ] && return 0
	power_status || return 0
	mkdir -p /var/lib/smbios
	mmio_dump 0x9ff00000 65536 -b > /var/lib/smbios/smbios2 2>/dev/null || :
	# ipmitool raw 0x2e 0x20 0x0a 0x3c 0x00 0x0b 0x5e 0x2a 0x3c 0x2a 0x00 0x00 | grep "0a 3c 00" >/dev/null 2>&1 || return 0
}

smbios() {
	[ ! -s "/var/lib/smbios/smbios2" ] && get_smbios
	[ ! -s "/var/lib/smbios/smbios2" ] && return 0
	dmidecode --from-dump /var/lib/smbios/smbios2 $@
}

debug_i2c() {
	[ -z "$1" ] && return 0
	local bus=$1
	echo $bus > /sys/module/i2c_aspeed/parameters/dump_debug_bus_id
	echo 1 > /sys/module/i2c_aspeed/parameters/dump_debug
}

debug_slave_i2c() {
	[ -z "$1" ] && return 0
	local bus=$1
	echo $bus > /sys/module/i2c_slave_mqueue/parameters/dump_debug_bus_id
	echo 1 > /sys/module/i2c_slave_mqueue/parameters/dump_debug
}

debug_off_i2c() {
	echo 0 > /sys/module/i2c_slave_mqueue/parameters/dump_debug
	echo 0 > /sys/module/i2c_aspeed/parameters/dump_debug
}

bootflashget() {
	local reg_out=""
	local i
	local reg
	local reg_out
	local socver=$(ast_getrev)
	[ "$socver" = "G6" ] && reg=$(devmem 0x1e620064) && reg=$(((reg & 0x10) / 16 + 1)) && echo spi$reg && return

	for i in 10 30 50; do
		reg=$(devmem 0x1e7850$i || echo "error")
		[ "$reg" = "error" ] && error "$reg"
		reg=$(((reg & 0x02) / 2 + 1))
		reg_out="$reg_out spi$reg"
	done
	local reg_final=$(echo $reg_out | grep -o spi2)
	[ -z "$reg_final" ] && reg_final=spi1
	echo $reg_final WDTs:$reg_out
}

# For backward compatibility
getbootflash() {
	bootflashget
}

# Unused, to be removed.
forcespi1() {
	# I will forget this in a week, so better comment
	# First write 1 to Clear Timeout Status Register
	# to drop the boot flash source bit to default (SPI1)
	# After that system will loose its' rootfs, so it
	# becomes impossible to use reboot.
	# Then reboot in a hard way using WDT1 with zero wait
	local i
	for i in 14 34 54; do
		devmem 0x1e7850$i 32 1
	done
	devmem 0x1e78500c 32 0x17; devmem 0x1e785004 32 0; devmem 0x1e785008 32 0x4755
}

# Unused, to be removed.
forcespi2() {
	devmem 0x1e78500c 32 0x97; devmem 0x1e785004 32 0; devmem 0x1e785008 32 0x4755
}

getbigmac() {
	ip link show dev eth0 | grep link | awk '{print $2}' | tr -d ":" | tr '[:lower:]' '[:upper:]'
}

getmacmin() {
	ip link show dev eth0 | grep link | awk '{print $2}' | tr -s ":" "-" | tr '[:upper:]' '[:lower:]'
}

bios_take() {
	[ "$host_cpu" = "none" ] && return 0
	gpioset $(gpiofind $BIOS_SEL)=0
}

bios_release() {
	[ "$host_cpu" = "none" ] && return 0
	gpioset $(gpiofind $BIOS_SEL)=1
}

detect_model() {
	local id=0
	local i
	local bit
	local id
	for i in 0 1 2 3; do
		bit=$(gpioget $(gpiofind BOARD_SKU_ID${i}))
		local exp=$((2**i))
		bit=$(($bit*$exp))
		id=$(($id+$bit))
	done
	echo $id
}

SPI_DEV="1e630000.spi"
SPI_PATH="/sys/bus/platform/drivers/spi-aspeed-smc"

bios_insert() {
	bios_eject
	bios_take
	# Enable SPI1 device
	local socver=$(ast_getrev)
	[ "$socver" = "G5" ] && devmem 0x1e6e207c l 0x2000 && devmem 0x1e6e2070 l 0x1000
	echo $SPI_DEV > $SPI_PATH/bind || :

	local i=5
	while true; do
		[ -h "$SPI_PATH/$SPI_DEV" ] && break
		i=$((i-1))
		[ $i -eq 0  ] && bios_release && return -1
		sleep 1
	done
	echo BIOS flash inserted
	return 0
}

bios_eject() {
	[ -h "$SPI_PATH/$SPI_DEV" ] && echo $SPI_DEV > $SPI_PATH/unbind
	local i=5
	while true; do
		[ ! -h "$SPI_PATH/$SPI_DEV" ] && break
		echo Still there
		i=$((i-1))
		[ $i -eq 0  ] && echo "Error" && bios_release && return -1
		sleep 1
	done
	echo BIOS flash removed
	bios_release
	return 0
}

bios_get_mtd() {
	local mtd
	mtd=$(cat /proc/mtd | grep $SPI_DEV | awk -F ":" '{print $1}')
	[ -n "$mtd" ] && echo $mtd && return 0
	cat /proc/mtd | grep "bios-partition" | awk -F ":" '{print $1}'
}

getver() {
	cat /etc/os-release  | grep YANDEX_VER | cut -f2 -d= | tr -d '"'
}

mbmod() {
	[ ! -s /etc/fru/fru.xml ] && echo ERROR && return 0
	[ -s /etc/mbmodel.txt ] && cat /etc/mbmodel.txt && return 0
	local model=$(cat /etc/fru/fru.xml | grep "BoardProductName" | tail -1 | awk -F ">" '{print $2}' | awk -F "-" '{print $1}')
	[ -n "$model" ] && echo $model > /etc/mbmodel.txt && echo $model && return 0
	echo ERROR && return 0
}

# ME commands
IPMB_OBJ="xyz.openbmc_project.Ipmi.Channel.Ipmb"
IPMB_PATH="/xyz/openbmc_project/Ipmi/Channel/Ipmb"
IPMB_INTF="org.openbmc.Ipmb"
IPMB_CALL="sendRequest yyyyay"
ME_CMD_RECOVER="1 0x2e 0 0xdf 4 0x57 0x01 0x00 0x01"
ME_CMD_RESET="1 6 0 0x2 0"

me_ipmb() {
	busctl call $IPMB_OBJ $IPMB_PATH $IPMB_INTF $IPMB_CALL $@
}

# Set fans to a given or safe level
setfans() {

    usage ()
    {
        echo "Usage: setfans (PWM in Percent), (Action Value), (Fan Number)"
        echo "SYS_FAN1 == 1"
        echo "SYS_FAN2 == 2"
        echo "CPU_FAN1 == 3"
        echo "CPU_FAN2 == 4"

        exit
    }

    local action=$2
    local fan_number=$3

    case ${action} in
        0)
            echo "Rotate each fan separatly"
                if [ ${model} == "g1" ]; then
                    pwmMax=255
                    if [ -z "$1" ]; then
                        pwmPercent=60
                    else
                        pwmPercent=$1
                    fi
                    pwmValue=$(($pwmMax * $pwmPercent / 100))

                    echo "$pwmValue" > /sys/bus/i2c/devices/14-0058/hwmon/hwmon1/pwm${fan_number}
                    echo "$pwmValue" > /sys/bus/i2c/devices/15-0058/hwmon/hwmon2/pwm${fan_number}

                    returnValue="$?"
                    if [ "${returnValue}" == 0 ]; then
                        echo "Success write $pwmPercent% pwm to $i."
                    else
                        echo "Failed write $pwmPercent% pwm to $i, error code: $returnValue."
                    fi
                else
                    local duty=$1
                    [ -z "$duty" ] && duty=100
                    local pwm=$((255 * $duty / 100))
                    [ $pwm -eq 0 -a "$duty" != "0" ] && pwm=255
                    [ $pwm -gt 255 ] && pwm=255
                    local hwmon="/sys/devices/platform/ahb/ahb:apb/1e786000.pwm-tacho-controller/hwmon/"
                    local socver=$(ast_getrev)
                    [ "$socver" = "G6" ] && hwmon="/sys/devices/platform/ahb/ahb:apb/1e610000.pwm-tacho-controller/hwmon/"
                    local hwmon_n=$(ls $hwmon)
                    hwmon="${hwmon}${hwmon_n}"

                    [ -f "$hwmon/pwm${fan_number}" ] && echo $pwm > $hwmon/pwm${fan_number}
                fi
            ;;

        $([ ! -z "${action}" ])|1)
            echo "Rotate all fans"
                if [ ${model} == "g1" ]; then
                    pwmMax=255
                    if [ -z "$1" ]; then
                        pwmPercent=60
                    else
                        pwmPercent=$1
                    fi
                    pwmValue=$(($pwmMax * $pwmPercent / 100))
                    for i in 1 2 3 4; do
                        echo "$pwmValue" > /sys/bus/i2c/devices/14-0058/hwmon/hwmon1/pwm${i}
                        echo "$pwmValue" > /sys/bus/i2c/devices/15-0058/hwmon/hwmon2/pwm${i}
                        returnValue="$?"
                        if [ "${returnValue}" == 0 ]; then
                            echo "Success write $pwmPercent% pwm to $i."
                        else
                            echo "Failed write $pwmPercent% pwm to $i, error code: $returnValue."
                        fi
                    done
                else
                    local duty=$1
                    [ -z "$duty" ] && duty=100
                    local pwm=$((255 * $duty / 100))
                    [ $pwm -eq 0 -a "$duty" != "0" ] && pwm=255
                    [ $pwm -gt 255 ] && pwm=255
                    local hwmon="/sys/devices/platform/ahb/ahb:apb/1e786000.pwm-tacho-controller/hwmon/"
                    local socver=$(ast_getrev)
                    [ "$socver" = "G6" ] && hwmon="/sys/devices/platform/ahb/ahb:apb/1e610000.pwm-tacho-controller/hwmon/"
                    local hwmon_n=$(ls $hwmon)
                    hwmon="${hwmon}${hwmon_n}"
                    local i
                    for i in 1 2 3 4; do
                        [ -f "$hwmon/pwm$i" ] && echo $pwm > $hwmon/pwm$i
                    done
                fi
            ;;
        *)
            echo "Unknown Fan Control action"
            usage
            ;;
    esac
}

ipmi_post() {
	ipmitool raw 0x32 0x73 0x0
}

snoop_irq_rate() {
	local ints
	local old_ints
	local diff=0
	local sum=0
	ints=0
	old_ints=$(cat /proc/interrupts  | grep lpc | awk '{print $2}')
	for i in 1 2 3; do
		ints=$(cat /proc/interrupts  | grep lpc | awk '{print $2}')
		diff=$((ints-old_ints))
		old_ints=$ints
		sum=$((sum+diff))
		sleep 1
	done
	sum=$((sum/3))
	echo $sum
}

snoop_irq_en() {
	# Same address/bitmask between G5 and G6
	local reg=$(devmem 0x1e789080)
	local newreg=$(($reg | 0x02))
	devmem 0x1e789080 l $newreg
}

snoop_irq_dis() {
	# Same address/bitmask between G5 and G6
	local reg=$(devmem 0x1e789080)
	local newreg=$(($reg & 0xfffffffd))
	devmem 0x1e789080 l $newreg
}

getpwm() {
	local pwms=""
	local socver=$(ast_getrev)
	[ "$socver" = "G6" ] && pwms=$(cat /sys/devices/platform/ahb/ahb:apb/1e610000.pwm-tacho-controller/hwmon/**/*_input 2>/dev/null) || \
		pwms=$(cat /sys/devices/platform/ahb/ahb\:apb/1e786000.pwm-tacho-controller/hwmon/**/*_input 2>/dev/null)
	echo PWM:$pwms
}

_eep_getlast() {
	local size=$(stat -c %s $FRU_PATH)
	[ -z "$size" ] && echo 0 && return
	[ $size -lt 1024 ] && echo 0 && return
	echo $((size-1))
}

eeprom_getruns() {
	local place=$(_eep_getlast)
	[ $place -eq 0 ] && return 0
	dd bs=1 skip=$place count=1 if=$FRU_PATH 2>/dev/null | hexdump -v -e '/1 "%d"'
}

eeprom_setruns() {
	local d=$1
	[ -z "$d" ] && return 0
	[ $d -gt 255 ] && d=255
	[ $d -lt 0 ] && d=0
	local place=$(_eep_getlast)

	[ $place -eq 0 ] && return 0
	d=$(printf "%02x" $d)
	local tmp=$(mktemp)
	printf "%b" "\x${d}" > $tmp
	dd bs=1 seek=$((place)) if=$tmp of=$FRU_PATH count=1 >/dev/null 2>&1 || :
	rm -rf $tmp >/dev/null 2>&1 || :
}

eeporm_resetruns() {
	eeprom_setruns 255
}

eeprom_getfactory() {
	local place=$(_eep_getlast)
	place=$((place-1))
	local flag=$(dd bs=1 skip=$place count=1 if=$FRU_PATH 2>/dev/null | hexdump -v -e '/1 "%d"')
	[ "$flag" = "170" ] && return 1
	return 0
}

_eep_setfactory() {
	local place=$(_eep_getlast)
	place=$((place-1))
	local tmp=$(mktemp)
	printf "%b" "255" > $tmp
	dd bs=1 seek=$((place)) if=$tmp of=$FRU_PATH count=1 >/dev/null 2>&1 || :
	rm -rf $tmp >/dev/null 2>&1 || :
}

eeprom_dropfactory() {
	local place=$(_eep_getlast)
	place=$((place-1))
	local tmp=$(mktemp)
	printf "%b" "170" > $tmp
	dd bs=1 seek=$((place)) if=$tmp of=$FRU_PATH count=1 >/dev/null 2>&1 || :
	rm -rf $tmp >/dev/null 2>&1 || :
}

ai1_init_redrivers() {
	local bus=6
	local addr=$1
	local selector=$2
	[ -z "$addr" -o -z "$selector" ] && return 0

	local set1="0x00 0x20 0x01 0x00 0x02 0x01 0x03 0x00 0x04 0x00 0x05 0x00 \
				0x06 0x18 0x07 0x01 0x08 0x04 0x09 0x00 0x0A 0x00 0x0B 0x70 \
				0x0C 0x00 0x0D 0x00 0x0E 0x08 0x0F 0x00 0x10 0xA8 0x11 0x80 \
				0x12 0x00 0x13 0x00 0x14 0x00 0x15 0x00 0x16 0x00 0x17 0xA8 \
				0x18 0x80 0x19 0x00 0x1A 0x00 0x1B 0x00 0x1C 0x00 0x1D 0x00 \
				0x1E 0xA8 0x1F 0x80 0x20 0x00 0x21 0x00 0x22 0x00 0x23 0x00 \
				0x24 0x00 0x25 0xA8 0x26 0x80 0x27 0x00 0x28 0x4C 0x29 0x00 \
				0x2A 0x00 0x2B 0x0C 0x2C 0x00 0x2D 0xA8 0x2E 0x80 0x2F 0x00 \
				0x30 0x00 0x31 0x00 0x32 0x0C 0x33 0x00 0x34 0xA8 0x35 0x80 \
				0x36 0x00 0x37 0x00 0x38 0x00 0x39 0x0C 0x3A 0x00 0x3B 0xA8 \
				0x3C 0x80 0x3D 0x00 0x3E 0x00 0x3F 0x00 0x40 0x0C 0x41 0x00 \
				0x42 0xA8 0x43 0x80 0x44 0x00 0x45 0x00 0x46 0x38 0x47 0x00 \
				0x48 0x05 0x49 0x00 0x4A 0x00 0x4B 0x00 0x4C 0x00 0x4D 0x00 \
				0x4E 0x00 0x4F 0x00 0x50 0x00 0x51 0x65 0x52 0x00 0x53 0x00 \
				0x54 0x00 0x55 0x00 0x56 0x10 0x57 0x44 0x58 0x21 0x59 0x00 \
				0x5A 0x54 0x5B 0x54 0x5C 0x00 0x5D 0x00 0x5E 0x00 0x5F 0x00 \
				0x60 0x00 0x62 0x00"

	local set2="0x00 0x28 0x01 0x00 0x02 0x01 0x03 0x00 0x04 0x00 0x05 0x00 \
				0x06 0x18 0x07 0x01 0x08 0x04 0x09 0x00 0x0A 0x00 0x0B 0x70 \
				0x0C 0x00 0x0D 0x00 0x0E 0x08 0x0F 0x00 0x10 0xA8 0x11 0x80 \
				0x12 0x00 0x13 0x00 0x14 0x00 0x15 0x00 0x16 0x00 0x17 0xA8 \
				0x18 0x80 0x19 0x00 0x1A 0x00 0x1B 0x00 0x1C 0x00 0x1D 0x00 \
				0x1E 0xA8 0x1F 0x80 0x20 0x00 0x21 0x00 0x22 0x00 0x23 0x00 \
				0x24 0x00 0x25 0xA8 0x26 0x80 0x27 0x00 0x28 0x4C 0x29 0x00 \
				0x2A 0x00 0x2B 0x0C 0x2C 0x00 0x2D 0xA8 0x2E 0x80 0x2F 0x00 \
				0x30 0x00 0x31 0x00 0x32 0x0C 0x33 0x00 0x34 0xA8 0x35 0x80 \
				0x36 0x00 0x37 0x00 0x38 0x00 0x39 0x0C 0x3A 0x00 0x3B 0xA8 \
				0x3C 0x80 0x3D 0x00 0x3E 0x00 0x3F 0x00 0x40 0x0C 0x41 0x00 \
				0x42 0xA8 0x43 0x80 0x44 0x00 0x45 0x00 0x46 0x38 0x47 0x00 \
				0x48 0x05 0x49 0x00 0x4A 0x00 0x4B 0x00 0x4C 0x00 0x4D 0x00 \
				0x4E 0x00 0x4F 0x00 0x50 0x00 0x51 0x65 0x52 0x00 0x53 0x00 \
				0x54 0x00 0x55 0x00 0x56 0x10 0x57 0x44 0x58 0x21 0x59 0x00 \
				0x5A 0x54 0x5B 0x54 0x5C 0x00 0x5D 0x00 0x5E 0x00 0x5F 0x00 \
				0x60 0x00 0x61 0x00"

	local set3="0x00 0x30 0x01 0x00 0x02 0x01 0x03 0x00 0x04 0x00 0x05 0x00 \
				0x06 0x18 0x07 0x01 0x08 0x04 0x09 0x00 0x0A 0x00 0x0B 0x70 \
				0x0C 0x00 0x0D 0x00 0x0E 0x08 0x0F 0x00 0x10 0xAD 0x11 0x80 \
				0x12 0x00 0x13 0x00 0x14 0x00 0x15 0x00 0x16 0x00 0x17 0xAD \
				0x18 0x80 0x19 0x00 0x1A 0x00 0x1B 0x00 0x1C 0x00 0x1D 0x00 \
				0x1E 0xAD 0x1F 0x80 0x20 0x00 0x21 0x00 0x22 0x00 0x23 0x00 \
				0x24 0x00 0x25 0xAD 0x26 0x80 0x27 0x00 0x28 0x4C 0x29 0x00 \
				0x2A 0x00 0x2B 0x0C 0x2C 0x00 0x2D 0xAD 0x2E 0x80 0x2F 0x00 \
				0x30 0x00 0x31 0x00 0x32 0x0C 0x33 0x00 0x34 0xAD 0x35 0x80 \
				0x36 0x00 0x37 0x00 0x38 0x00 0x39 0x0C 0x3A 0x00 0x3B 0xAD \
				0x3C 0x80 0x3D 0x00 0x3E 0x00 0x3F 0x00 0x40 0x0C 0x41 0x00 \
				0x42 0xAD 0x43 0x80 0x44 0x00 0x45 0x00 0x46 0x38 0x47 0x00 \
				0x48 0x05 0x49 0x00 0x4A 0x00 0x4B 0x00 0x4C 0x00 0x4D 0x00 \
				0x4E 0x00 0x4F 0x00 0x50 0x00 0x51 0x65 0x52 0x00 0x53 0x00 \
				0x54 0x00 0x55 0x00 0x56 0x10 0x57 0x44 0x58 0x21 0x59 0x00 \
				0x5A 0x54 0x5B 0x54 0x5C 0x00 0x5D 0x00 0x5E 0x00 0x5F 0x00 \
				0x60 0x00 0x61 0x00"

	local set4="0x00 0x38 0x01 0x00 0x02 0x01 0x03 0x00 0x04 0x00 0x05 0x00 \
				0x06 0x18 0x07 0x01 0x08 0x04 0x09 0x00 0x0A 0x00 0x0B 0x70 \
				0x0C 0x00 0x0D 0x00 0x0E 0x08 0x0F 0x00 0x10 0xAD 0x11 0x80 \
				0x12 0x00 0x13 0x00 0x14 0x00 0x15 0x00 0x16 0x00 0x17 0xAD \
				0x18 0x80 0x19 0x00 0x1A 0x00 0x1B 0x00 0x1C 0x00 0x1D 0x00 \
				0x1E 0xAD 0x1F 0x80 0x20 0x00 0x21 0x00 0x22 0x00 0x23 0x00 \
				0x24 0x00 0x25 0xAD 0x26 0x80 0x27 0x00 0x28 0x4C 0x29 0x00 \
				0x2A 0x00 0x2B 0x0C 0x2C 0x00 0x2D 0xAD 0x2E 0x80 0x2F 0x00 \
				0x30 0x00 0x31 0x00 0x32 0x0C 0x33 0x00 0x34 0xAD 0x35 0x80 \
				0x36 0x00 0x37 0x00 0x38 0x00 0x39 0x0C 0x3A 0x00 0x3B 0xAD \
				0x3C 0x80 0x3D 0x00 0x3E 0x00 0x3F 0x00 0x40 0x0C 0x41 0x00 \
				0x42 0xAD 0x43 0x80 0x44 0x00 0x45 0x00 0x46 0x38 0x47 0x00 \
				0x48 0x05 0x49 0x00 0x4A 0x00 0x4B 0x00 0x4C 0x00 0x4D 0x00 \
				0x4E 0x00 0x4F 0x00 0x50 0x00 0x51 0x65 0x52 0x00 0x53 0x00 \
				0x54 0x00 0x55 0x00 0x56 0x10 0x57 0x44 0x58 0x21 0x59 0x00 \
				0x5A 0x54 0x5B 0x54 0x5C 0x00 0x5D 0x00 0x5E 0x00 0x5F 0x00 \
				0x60 0x00 0x61 0x00"

	local _cmds=$set1
	[ "$selector" = "2" ] && _cmds=$set2
	[ "$selector" = "3" ] && _cmds=$set3
	[ "$selector" = "4" ] && _cmds=$set4
	local _reg=""
	local _cmd=""
	local a
	for a in $_cmds; do
		[ -z "$_reg" ] && _reg=$a && continue
		[ -n "$_reg" ] && _cmd=$a
		i2cset -y $bus $addr $_reg $_cmd >/dev/null 2>&1 || :
		_reg=""
		_cmd=""
	done
}

drop_factory()
{
	echo Locking P2A
	p2a_lock || :
}

# Used only on AST2500
enable_vga_scu()
{
	local reg
	reg=$(devmem 0x1e6e2094 || :)
	reg=$(($reg & 0xfffffffd))
	reg=$(($reg | 0x01))
	devmem 0x1e6e2094 32 $reg || :
}

dump_throttle_gpio()
{
	local str
	for i in H3 B3 E2 E3 E4 E6; do val=$(gpioget $(gpiofind $i)); str="$str$i:$val "; done
	echo $str
}

reset_dcdc()
{
	echo 0x60 > /sys/bus/i2c/devices/i2c-23/delete_device || :
	i2ctransfer -y 23 w1@0x60 0x03 || :
}

# SGPIO CPLD interaction part
# Used only on AST2500
sgpio_start()
{
	# Set GPIO line to select SGPIO
	gpioset $(gpiofind BPB_CPLD_PROGRAMMING_N)=1 >/dev/null 2>&1 || :

	# SCU Enable SGPIO Master
	local scu=$(devmem 0x1e6e2084)
	scu=$((scu|0xFF00))
	devmem 0x1e6e2084 32 $scu

	# 100kHz = 007a
	# 500kHz = 0018
	# 1MHz = 000b
	# 48k 0x00ff0281
	devmem 0x1e780254 32 0x01000281
}

# Used only on AST2500
sgpio_stop()
{
	devmem 0x1e780254 32 0x01000280
}

# Used only on AST2500
sgpio_set()
{
	cmd=$1
	devmem 0x1e780200 32 0x00${cmd}0000
	sgpio_start
}

# Used only on AST2500
sgpio_get()
{
	local abcd=$(devmem 0x1e780200)
	local efgh=$(devmem 0x1e78021c)
	local ij=$(devmem 0x1e780238)

	# Inverse SGPIO received data
	local a=$(($((ij&0xFF00)) >> 8))
	local b=$((ij&0xFF))
	local c=$(($((efgh&0xFF000000)) >> 24))
	local d=$(($((efgh&0xFF0000)) >> 16))
	local e=$(($((efgh&0xFF00)) >> 8))
	local f=$((efgh&0xFF))
	local g=$(($((abcd&0xFF000000)) >> 24))
	local h=$(($((abcd&0xFF0000)) >> 16))
	local i=$(($((abcd&0xFF00)) >> 8))
	local j=$((abcd&0xFF))

	# Combine to Usercode, and shift 1-bit cuz SGPIO CLK Timing
	local usercode=$(($(($((j << 72)) + $((i << 64)) + $((h << 56)) + $((g << 48)) + $((f << 40)) + $((e << 32)) + $((d << 24)) + $((c << 16)) + $((b << 8)) + $((a))))<<1))

	# Get CPLD Version
	local version=$(($((usercode&0xFF00000000)) >> 32))
	local cmd=$(($((usercode&0xFF0000)) >> 16))

	echo $((version)) 16op | dc 2>/dev/null || return 1
}

# Used only on AST2500
sgpio_cpld_init()
{
	# Frequency is 50kHz
	# 80-bit cycle takes 1.6mS
	# 0x20 must be sent at least 15 times that takes 24mS
	# 0.1 second sleep generates 62 80-bits cycles that should be pretty enough
	sgpio_set 20
	sleep 0.1
}

# Used only on AST2500
sgpio_get_ver()
{
	for i in 1 2 3 4 5 6; do
		sgpio_set $1
		ver=$(sgpio_get 2>/dev/null) && echo $1 ver $ver && break
	done
}

# Used only on AST2500
sgpio_get_cplds_ver()
{
	sgpio_cpld_init
	for i in 05 06; do
		sgpio_get_ver $i
	done
	sgpio_stop
}

lm5066i_setmask()
{
	i2ctransfer -y $1 w3@$2 0xd8 0x06 0xfd >/dev/null 2>&1 || :
	i2ctransfer -y $1 w1@$2 0x03 >/dev/null 2>&1 || :
}

lm5066i_rm()
{
	local bus=$(echo $1 | sed -e 's/0x//g' || :)
	local addr=$(echo $2 | sed -e 's/0x//g' || :)
	[ ! -e "/sys/bus/i2c/devices/$bus-00$addr" ] && return 0
	echo 0x$addr > /sys/bus/i2c/devices/i2c-$bus/delete_device || :
}

lm5066i_add()
{
	local bus=$(echo $1 | sed -e 's/0x//g' || :)
	local addr=$(echo $2 | sed -e 's/0x//g' || :)
	[ -z "$bus" -o -z "$addr" ] && return 0

	local i=0
	while true; do
		lm5066i_rm $bus $addr
		sleep 1
		lm5066i_setmask $bus 0x$addr
		echo lm5066i05 0x$addr > /sys/bus/i2c/devices/i2c-$bus/new_device || :
		sleep 1
		local hwmon=$(ls /sys/bus/i2c/devices/${bus}-00${addr}/hwmon/)
		[ -e "/sys/bus/i2c/devices/${bus}-00${addr}/hwmon/$hwmon/power1_average" ] && return
		i=$((i+1))
		[ $i -eq 5 ] && return 0
	done
}

dcdc_getvendor()
{
	local bus=$1
	local addr=$2
	local rc=$(i2ctransfer -y $bus w1@$addr 0x99 r3 2>&1 || :)
	echo $rc | grep -q "0x04 0x42 0x45" && echo "bel" > /etc/dcdc_model.txt && return 0
	echo $rc | grep -q "0x06 0x44 0x45" && echo "delta" > /etc/dcdc_model.txt && return 0
	echo $rc | grep -q "0x08 0x41 0x72" && echo "artesyn" > /etc/dcdc_model.txt && return 0
	echo "unknown" > /etc/dcdc_model.txt
}

pmbus_reset_bus()
{
	gpioset $(gpiofind BMC_RST_I2C_DEV_1)=0
	sleep 0.2
	gpioset $(gpiofind BMC_RST_I2C_DEV_1)=1
}

get_bios_version()
{
	a=$(ipmitool raw 0x2e 0x21 0x0a 0x3c 0x00 0x019 0x01 0x00); for i in $(echo $a); do printf "\x$i"; done; echo
}

get_ipv6()
{
	ip -6 addr show | grep -v "scope link" | grep "scope global" | tail -1 | sed -e's/^.*inet6 \([^ ]*\)\/.*$/\1/;t;d'
}

check_bus()
{
	local bus=$1
	[ ! -d "/sys/bus/i2c/devices/i2c-$bus" ] && echo No such bus && return 0
	local base_addr=$(find /sys/devices/platform -name i2c-$bus | grep -v i2c-dev | sed -n 's/.*\/\([^/]*\)\.i2c-bus.*/\1/p')
	base_addr=$(printf "0x%x" $((0x$base_addr + 0x14)))

	local n=0
	local rc=0
	while true; do
		local a=$(devmem $base_addr)
		[ -z "$a" ] && continue
		a=$((a>>16 & 0x01))
		[ $a -eq 0 ] && echo Bus $bus OK && return 0
		n=$((n+1))
		[ $n -eq 10 ] && rc=1 && echo Bus $bus FAIL && return 1
	done
}

fix_asset_tag()
{
	local tmp=$(mktemp)
	ipmitool fru print 0 > $tmp
	local product_serial=$(grep "Product Serial" $tmp | awk '{print $4}')
	local asset_tag=$(grep "Product Asset Tag" $tmp | awk '{print $5}')
	[ "$product_serial" != "" -a "$asset_tag" = "" ] && \
		ipmitool fru edit 0 field product 5 "$product_serial" >/dev/null 2>&1 && echo Fixed asset tag || :
	rm -rf $tmp >/dev/null 2>&1 || :
}

usb_get_free_port()
{
	local ports="p2 p3 p4 p5"
	local i
	for i in /sys/kernel/config/usb_gadget/*; do
		local port=$(cat $i/UDC | sed 's/1e6a0000.usb-vhub://g')
		[ -z "$port" ] && continue
		ports=$(echo $ports | sed "s/$port//g")
	done
	for i in $ports; do echo $i; break; done
}

# Unused. To be removed
reset_chip()
{
	devmem 0x1e78500c 32 0x37; devmem 0x1e785004 32 0; devmem 0x1e785008 32 0x4755
}

# Unused. To be removed
uart1_clock_en()
{
	local val=$(devmem 0x1e6e200c)
	local val2=$(($val & 0xfffe7fff))
	devmem 0x1e6e200c w $val2 || :
}

wdt3_getaddr()
{
	local socver=$(ast_getrev)
	local addr="0x1e78504"
	[ "$socver" = "G6" ] && addr="0x1e78508"
	echo $addr
}

wdt3_getcnt()
{
	local addr=$(wdt3_getaddr)
	local cnt=$(devmem ${addr}0 || :)
	cnt=$(($cnt / 1000000))
	echo $cnt
}

wdt3_stop()
{
	local addr=$(wdt3_getaddr)
	devmem ${addr}c 32 0x00 || :
}

wdt_update_resets()
{
	local data="0x1e78501c 32 0x023cd773 0x1e78503c 32 0x023cd773 0x1e78505c 32 0x023cd773"
	local socver=$(ast_getrev)
	[ "$socver" = "G6" ] && data="0x1e78501c 32 0x030f17f1 0x1e785020 32 0x03ffd7f1 0x1e78505c 32 0x030f17f1 0x1e785060 32 0x03ffd7f1 0x1e78509c 32 0x030f17f1 0x1e7850a0 32 0x03ffd7f1"
	local addr=""
	local len=""
	local val=""
	local i=0
	local v=""
	for v in $data; do
		[ $i -eq 0 ] && addr=$v && i=$((i+1)) && continue
		[ $i -eq 1 ] && len=$v && i=$((i+1)) && continue
		[ $i -eq 2 ] && val=$v && i=0 && devmem $addr $len $val
	done
}

uart_set_route()
{
	[ -z "$1" -o -z "$2" ] && return 0
	local port1="$1"
	local port2="$2"
	local path="/sys/bus/platform/drivers/aspeed-uart-routing/1e78909c.uart_routing"
	local socver=$(ast_getrev)
	[ "$socver" = "G6" ] && path="/sys/bus/platform/drivers/aspeed-uart-routing/1e789098.uart-routing"
	echo -n $port1 > $path/$port2 || :
	echo -n $port2 > $path/$port1 || :
}

disable_spi_passthrough()
{
	local socver=$(ast_getrev)
	# There is no SPI pass-through in G6
	[ "$socver" = "G6" ] && return 0
	devmem 0x1e6e207c 32 0x2000 || :
}

persistent_format()
{
	mount | grep -q "/persistent" && return 0
	local char=$(mtdname2dev persistent)
	flash_eraseall $char >/dev/null 2>&1 || :
	return 0
}

persistent_mount()
{
	mount | grep -q "/persistent" && return 0
	local block=$(mtdname2block persistent)
	[ -z "$block" ] && return 1
	mount -t jffs2 $block /persistent >/dev/null 2>&1 || :
	mount | grep -q "/persistent"
	[ $? -ne 0 ] && persistent_format && persistent_mount
	return 0
}

persistent_umount()
{
	umount /persistent >/dev/null 2>&1 || :
}

persistent_save()
{
	rm -f /tmp/rw.tar >/dev/null 2>&1 || :
	tar -C /run/initramfs/rw/cow/ -cf /tmp/rw.tar --exclude=upgraded.flg etc home var usr/share || :
	[ -f /tmp/rw.tar -a -d /run/initramfs/rw/cow/lib ] && tar -C /run/initramfs/rw/cow/ --append -f /tmp/rw.tar lib || :
	[ ! -f /tmp/rw.tar ] && return 1
	persistent_mount || return 1
	rm /persistent/rw.tar >/dev/null 2>&1 || :
	mv /tmp/rw.tar /persistent/ || :
	persistent_umount || :
	return 0
}

persistent_restore()
{
	[ -f "/etc/persistent_restored" ] && return 0
	persistent_mount || return 1
	tar -C / -xf /persistent/rw.tar >/dev/null 2>&1 || return 1
	persistent_umount || :
	touch /etc/persistent_restored || :
	sync || :
	systemctl restart --no-block systemd-networkd.service || :
	return 0
}

adc_lower_rate()
{
	[ -z "$1" ] && return 0
	echo $1 > /sys/bus/iio/devices/iio\:device0/in_voltage_sampling_frequency || :
}

reset_all_mux()
{
    echo Resetting I2C muxes
    gpioset $(gpiofind BMC_RST_I2C_DEV_1)=0 || :
    sleep 0.5 || :
    gpioset $(gpiofind BMC_RST_I2C_DEV_1)=1 || :
    sleep 0.5 || :
}

al2s_killpower()
{
	i2cset -y 6 0x40 0x01 0x00
}

al2s_bios_switch()
{
	gpioset $(gpiofind BIOS_BACKUP_SEL)=$1
}

wdt_reboot()
{
	echo "Stopping WDTs..."
	local rev=$(ast_getrev || :)
	if [ "$rev" = "G5" ]; then
		devmem 0x1e78500c 32 0 || :
		devmem 0x1e78502c 32 0 || :
		devmem 0x1e78504c 32 0 || :
	fi
	if [ "$rev" = "G6" ]; then
		devmem 0x1e78500c 32 0 || :
		devmem 0x1e78504c 32 0 || :
		devmem 0x1e78508c 32 0 || :
		devmem 0x1e7850cc 32 0 || :
		devmem 0x1e78510c 32 0 || :
		devmem 0x1e78514c 32 0 || :
		devmem 0x1e78518c 32 0 || :
		devmem 0x1e7851cc 32 0 || :
	fi

	sled_hb_hb || :

	echo "Setting up WDT1 for ARM reboot"
	# Set timeout to 5 seconds
	devmem 0x1e785004 32 0x4c4b40 || :
	# Load counter reload value to counter register
	devmem 0x1e785008 32 0x4755 || :
	# Enable WDT1, reset ARM core only, use first flash (AST2500 only),
	# disable interrupt,  use 1MHz clock (AST2500 only)
	devmem 0x1e78500c 32 0x53 || :

	echo -n "WDT1CR " || :
	devmem 0x1e78500c || :

	echo "Last heart beats following..."

	while true; do
		echo "KNOCK knock..."
		sleep 1
	done

	# Should never be executed except it's a miracle
	echo "ACHTUNG!!!! ZOMBIE ATTACK!!!"
}

factory_reset()
{
	persistent_mount || return 1
	rm -rf /persistent/* >/dev/null 2>&1 || :
	persistent_umount || :
	rm -rf /run/initramfs/rw/cow/* >/dev/null 2>&1 || :
	tar -C /run/initramfs/rw/cow/ -xzf /usr/share/rwfs/rwfs.tgz >/dev/null 2>&1 || :

	wdt_reboot

	return 0
}
