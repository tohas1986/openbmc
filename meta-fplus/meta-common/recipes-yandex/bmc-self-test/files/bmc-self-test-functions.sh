#!/bin/sh

source /usr/share/openrack/functions

HOSTNAME=$model
LOGFILE=/usr/share/bmc-self-test/bmc-self-test_$HOSTNAME.json

add_start()
{
    printf '[\n' >> $LOGFILE
}

add_end()
{
    printf ']\n' >> $LOGFILE
}

clear_old_logs()
{
    rm $LOGFILE
}

get_uptime()
{
    [ -f "/usr/bin/uptime" ] && UPTIME=`/usr/bin/uptime`

    printf '{\n    "HOSTNAME":"%s",\n    "UPTIME":"%s"\n},\n' "$HOSTNAME" "$UPTIME" >> $LOGFILE

    return 0
}

get_ipv6()
{
    IPV6=$(ip addr show dev eth0 | sed -e 's/^.*inet6 \([^ ]*\)\/.*$/\1/;t;d' | sed -n '1 p')
    
    printf '{\n    "IPv6":"%s"\n},\n' "$IPV6" >> $LOGFILE

    return 0
}

get_date()
{
    DATE=`/bin/date`

    printf '{\n    "DATE":"%s"\n},\n' "$DATE" >> $LOGFILE
}

get_hwwbus_status()
{
    curl -qs https://hwwbus.haas.yandex-team.ru:443 -o /dev/null
    [[ $? == 0 ]] && STATUS="PASS"
    [[ $? != 0 ]] && STATUS="FAIL"

    printf '{\n    "HWWBUS connection":"%s"\n},\n' "$STATUS" >> $LOGFILE
}

check_fans()
{
    #Disable PID control
    systemctl stop --no-block phosphor-pid-control >/dev/null 2>&1

    #Setting minimal value to FANs
    setfans 10

    local TEMP=0
    for PCT in $(seq 20 10 90)
    do
        setfans ${PCT}
        
        #Wait till rotation reach set speed
        sleep 2

        if [[ $HOSTNAME == "g1" ]]; then
		for DEVICE in $(ls /sys/bus/i2c/devices/ | grep 0058)
		do	
			for FILE in {1..4}
			do			
	                       FANPATH="/sys/bus/i2c/devices/${DEVICE}/hwmon/**/fan${FILE}_input)"
         	               PWMPATH="/sys/bus/i2c/devices/${DEVICE}/hwmon/**/pwm${FILE})"

				if [ -f $FANPATH ]; then
		            		INPUT_TACH=$(cat ${FANPATH})
	                    		PWM=$(cat ${PWMPATH})

                            		[[ "$INPUT_TACH" > "$TEMP" ]] && TEMP=$INPUT_TACH
                            		[[ "$INPUT_TACH" < "$TEMP" ]] && printf '{\n    "FANs":"%s",\n},\n' "FAILED" >> $LOGFILE && systemctl start phosphor-pid-control >/dev/null 2>&1 && return 0
				fi
            		done
		done
        else
            for FILE in {1..8}
            do
                FANPATH="/sys/devices/platform/ahb/ahb:apb/1e786000.pwm-tacho-controller/hwmon/**/fan${FILE}_input"
                PWMPATH="/sys/devices/platform/ahb/ahb\:apb/1e786000.pwm-tacho-controller/hwmon/hwmon*/pwm${FILE}"
                
                if [ -f $FANPATH ]; then

                    INPUT_TACH=$(cat ${FANPATH})
                    PWM=$(cat ${PWMPATH})
                    
                    [[ "$INPUT_TACH" > "$TEMP" ]] && TEMP=$INPUT_TACH
                    [[ "$INPUT_TACH" < "$TEMP" ]] && printf '{\n    "FANs":"%s",\n},\n' "FAILED" >> $LOGFILE && systemctl start phosphor-pid-control >/dev/null 2>&1 && return 0
                fi
            done
        fi
    done

    printf '{\n    "FANs":"%s"\n},\n' "PASSED" >> $LOGFILE

    #Enable PID control
    systemctl start --no-block phosphor-pid-control
}

check_services()
{
    SERVICELIST=(bmcweb.service ntpd.service phosphor-pid-control xyz.openbmc_project.EntityManager)
    for VALUE in "${SERVICELIST[@]}"
    do
        STATUS=$(systemctl status $VALUE | grep Active | sed 's/     Active: //g')
        printf '{\n    "SERVICE":"%s",\n    "STATUS":"%s"\n},\n' "$VALUE" "$STATUS" >> $LOGFILE
    done
}

toggle_led()
{
    # Toggle the state of identify LED Group

    SERVICE="xyz.openbmc_project.LED.Controller.identify"
    INTERFACE="xyz.openbmc_project.Led.Physical"
    object="/xyz/openbmc_project/led/physical/identify"
    PROPERTY="State"

    # Get current state
    object=$(busctl tree $SERVICE --list | grep identify)
    state=`busctl get-property $SERVICE $object $INTERFACE $PROPERTY | awk '{print $2}'`

    if [ "$state" == "\"xyz.openbmc_project.Led.Physical.Action.Off\"" ]; then
        target='xyz.openbmc_project.Led.Physical.Action.On'
    else
        target='xyz.openbmc_project.Led.Physical.Action.Off'
    fi

    # Set target state
    busctl set-property $SERVICE $object $INTERFACE $PROPERTY s $target
    
    [[ $? == 0 ]] && STATUS="PASS"
    [[ $? != 0 ]] && STATUS="FAIL"

    printf '{\n    "ID LED":"%s"\n}\n' "$STATUS" >> $LOGFILE
}
