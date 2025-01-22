#!/bin/sh

echo "Try to start dimmserial.service to refresh Samsung Inventory"

# Check certs and add it to /etc if necessary
if [ ! -f /etc/ssl/certs/authority/CA-cert.pem ]; then
	cp /usr/sbin/CA-cert.pem /etc/ssl/certs/authority/
	if [ ! -f /etc/nslcd/certs/cert.pem ]; then
		cp /usr/sbin/cert.pem /etc/nslcd/certs/
	fi
fi


#Check Product Serial Number and match it to Asset Tag
pserial=$(ipmitool fru |grep -m1 "Product Serial" |cut -d ': ' -f 2)
echo $pserial
asset=$(ipmitool fru |grep -m1 "Product Asset Tag" |cut -d ': ' -f 2)
echo $asset

if [ "$pserial" != "$asset" ]; then
    echo "The numbers are not equal. Try to change Asset Tag"
    ipmitool fru edit 0 field product 5 $pserial
fi

sleep 30s
/usr/sbin/dimmserial
echo "Script ended"