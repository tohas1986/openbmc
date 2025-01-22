#!/bin/bash

log() {
	local ts=$(date +%Y-%m-%d_%H:%M)
	echo $ts : $@
}

PLATFORMS="ai1 gd1 my81 mz81 mzb2 my62 g1 gd2 evb2600 som2600v1"
FWDIR=/work/openbmc_fw

rm -f $FWDIR/*.tar >/dev/null 2>&1 || :

debug=true
[ -n "$1" ] && debug=false

oldpath=$PATH
obmcroot=$(pwd  | sed 's/\/meta-yandex//g')
for i in $PLATFORMS; do
	cd $obmcroot/
	source ./setup $i
	cd $obmcroot/build/$i

	export PATH=$obmcroot/scripts:$obmcroot/poky/bitbake/bin:$oldpath
	export BBPATH=$obmcroot/build/$i
	export BUILDDIR=$obmcroot/build/$i
	log "Building for $i"

	# bitbake -c cleansstate yandex-ipmi-oem || :
	# bitbake -c cleansstate rmt-parser || :
	# bitbake -c cleansstate nvme-cooler || :
	# cmd="bitbake obmc-phosphor-image -k"
 	bitbake -c cleansstate rmt-parser hwwbus-hb yandex-ipmi-oem id-button nvme-cooler image-yandex obmc-libjtag lldp-forwarder lpc-cmds default-fru || : 
	[ $debug = "false" ] && debug_out="/dev/null" || debug_out="/dev/stdout"
	rc=true
	# rm -rf tmp
	bitbake image-ext image-yandex || rc=false
	if [ $rc ]; then
		cp $BUILDDIR/tmp/deploy/images/$i/$i*.fullflash*.tar $FWDIR || :
		cp $BUILDDIR/tmp/deploy/images/$i/$i*.upgrade*.tar $FWDIR || :
		cp $BUILDDIR/tmp/deploy/images/$i/$i*.fullflash*.tar $FWDIR || :
		cp $BUILDDIR/tmp/deploy/images/$i/$i*.upgrade*.tar $FWDIR || :
		log "Job is done for $i"
	else
		log "Job failed for $i"
	fi

done
