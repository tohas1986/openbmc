#!/bin/bash -e

set +e

[ -e "/run/board-setup.lck" ] && exit 0

# Prevent second run
touch /run/board-setup.lck

. /usr/share/openrack/functions

# TODO REMOVE THIS IN PRODUCTION FIXME
# Create /dev/mem
[ ! -e "/dev/mem" ] && mknod -m 660 /dev/mem c 1 1 && chown root:kmem /dev/mem

# Light up the green light
sled_on_green

# Reset WDT3
echo Stopping WDT3, $(wdt3_getcnt) seconds left
wdt3_stop

# Wait for U-Boot environment appear
i=0
while ! test -e /dev/mtd/u-boot-env; do
		echo "No u-boot-env yet"
		sleep 1
		i=$((i+1))
		[ $i -gt 10 ] && break
done

# Restore configuration
# Moved to initramfs

# Save clock to RTC
echo Setting RTC
year=$(date +%Y)
if [ -z "$year" -o $year -lt 2021 ]; then
	blddate=$(cat /etc/os-release 2>&1 | grep BUILD_TIMESTAMP | awk -F "\"" '{print $2}')
	[ -n "$blddate" ] && echo "Cold start: Setting time to $blddate" &&  date -s "$blddate" && hwclock -w || :
fi

# Do FRU job
if [ ! -f "$FRU_PATH" ]; then
	echo Instantiating FRU EEPROM
	echo $fru_type 0x$fru_addr > /sys/bus/i2c/devices/i2c-${fru_bus}/new_device 2>&1 || :
fi

# Check FRU. If it's empty - save default structure
_fru=$(dd if=$FRU_PATH bs=1 count=512 2>/dev/null | strings)
[ -z "$_fru" ] && echo "Detected empty FRU, writing default" && dd if=/usr/share/openrack/empty_fru.bin of=$FRU_PATH >/dev/null 2>&1

[ -e "/etc/fru" ] || mkdir -p /etc/fru

echo OpenBMC FRU header generated
fru_get_xml
fru_generate_header

# In case we have AMI FRU - back it up (in fru_write_to_eeprom) and generate new one
if fru_is_ami; then
	echo Backing up EERPOM header
	fru_write_to_eeprom
fi

# Get MAC address
env_mac=$(fw_printenv 2>&1 | grep ethaddr | sed -e 's/ethaddr=//g' | tr '[:lower:]' '[:upper:]')
eep_mac=$(strings -n 1 $FRU_PATH | grep MAC= | sed -e 's/MAC=//g' | tr '[:lower:]' '[:upper:]')
if_mac=$(ip link show dev eth0 | grep ether | awk '{print $2}' | tr '[:lower:]' '[:upper:]')
echo Got MACs: U-Boot: \"$env_mac\" FRU: \"$eep_mac\" IF: \"$if_mac\"

if [ -n "$eep_mac" -a "$eep_mac" != "$env_mac" ]; then
	echo Writing $eep_mac to U-Boot environment
	fw_setenv ethaddr $eep_mac
fi

if [ -n "$eep_mac" -a "$eep_mac" != "$if_mac" ]; then
	echo Setting eth0 HWADDR to $eep_mac
	ip link set eth0 down
	ip link set eth0 address $eep_mac up
fi



# Set up reset controller in case it was bypassed in U-Boot
echo Update SoC reset controller settings
wdt_update_resets || :

# Set hostname
bm=$(getmacmin)
h=$(hostname)
[ -z "$h" ] && h="$model"
[ -n "$bm" -a -n "$model" -a "$h" = "$model" ] && echo $bm > /etc/hostname && hostname $bm && echo "Set hostname to $bm"

# Update smbios
if [ "$host_cpu" != "none" ]; then
	echo Getting SMBIOS table
	mkdir -p /var/lib/smbios
	get_smbios
fi

# Populate ProductID
mkdir -p /var/cache/private
case "$model" in
	"mz81") prod_id=7777
	;;
	"my81") prod_id=7778
	;;
	"ai1") prod_id=7779
	;;
	"mzb2") prod_id=7776
	;;
	"gd1") prod_id=7775
	;;
	"my62") prod_id=7774
	;;
	"g1") prod_id=7773
	;;
	"gd2") prod_id=7772
	;;
	"evb2600") prod_id=7771
	;;
	"mr92") prod_id=7770
	;;
	"som2600v1") prod_id=7769
	;;
	"som2600v1-sec") prod_id=7769
	;;
	*) prod_id=0
	;;
esac

echo Motherboard model: $model ProductID: $prod_id
[ -s "/var/cache/private/prodID" ] || echo $prod_id > /var/cache/private/prodID || :

# Enable common services
if [ "$model" != "unknown" ]; then
	systemctl enable --no-block inventory-job.service || :
fi

# Run custom script
echo Running custom startup script
if [ -x "/usr/share/openrack/custom/${model}" ]; then
	/usr/share/openrack/custom/${model} || :
else
	/usr/share/openrack/custom/unknown || :
fi

# P2A lock status
echo "Current lock status: $(p2a_status)"

# Override AST2500 ADC bug
socver=$(ast_getrev)
[ "$socver" = "G5" ] && echo "Lowering ADC sampling rate" && adc_lower_rate 3220

# Run cauth-updater via systemd-run
#systemctl status cauth-updater.service >/dev/null 2>&1 && echo "Scheduling cauth-updater" && systemctl start --no-block cauth-updater.service || :

# Check runs
_runs=$(eeprom_getruns)
[ $_runs -eq 255 ] && echo "Clear motherboard detected, settings factory mode counter to 10" && eeprom_setruns 10
_runs=$(eeprom_getruns)
if [ $_runs -ne 0 ]; then
	[ $_runs -eq 1 ] && echo "10 runs passed"
	[ $_runs -gt 1 ] && _runs=$((_runs-1)) && echo "$_runs in factory mode left" && eeprom_setruns $_runs
else
	echo "Runs is 0, EEPROM tampering detected, setting runs to 1"
	eeprom_setruns 1 || :
fi

# Lock P2A
p2a_lock
p2a_status

# Check factory mode and start dropbear if enabled
eeprom_getfactory && touch /var/run/factory_mode

# Start dropbear in factory mode
[ -f "/var/run/factory_mode" ] && systemctl start --no-block dropbear || :

# Fix users membership in groups
echo Fixing users membership
usermod -G ipmi,redfish,priv-admin,sudo,web,adm root || :
groupmod -U root adm || :
usermod -G ipmi,redfish,priv-admin,web ADMIN || :

# Check certs and add it to /etc if necessary
sleep 10s
if [ ! -f /etc/ssl/certs/authority/CA-cert.pem ]; then
	cp /usr/share/openrack/CA-cert.pem /etc/ssl/certs/authority/
	if [ ! -f /etc/nslcd/certs/cert.pem ]; then
		cp /usr/share/openrack/cert.pem /etc/nslcd/certs/
	fi
fi
