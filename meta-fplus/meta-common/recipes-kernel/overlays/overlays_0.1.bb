DESCRIPTION = "Device tree overlays"
PR = "r0"

inherit skeleton-rev

SRC_URI += "file://LICENSE"
SRC_URI += "file://veeprom.dts"
SRC_URI += "file://slave-eeprom.dts"
SRC_URI += "file://gd1.dts"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=fa818a259cbed7ce8bc2a22d35a464fc"

DEPENDS = "linux-aspeed bash "
RDEPENDS:${PN} += "bash "

S = "${WORKDIR}"

do_compile() {
    BUILDDIR="${WORKDIR}/../../../../.."
    dtc=$BUILDDIR/tmp/work/*-linux-gnueabi/linux-aspeed/5.*/linux-*-standard-build/scripts/dtc/dtc
    inc=$(readlink -f $BUILDDIR/tmp/work-shared/*/kernel-source)
    for dts in *.dts; do
       dt=${dts%%.dts}
       ${CPP} -nostdinc -I${inc}/arch/arm/boot/dts -I${inc}/include -undef -D__DTS__  -x assembler-with-cpp -o ${WORKDIR}/$dts.tmp ${WORKDIR}/$dts
       $dtc -I dts -O dtb -o ${WORKDIR}/$dt.dtbo -b 0 -H epapr -@ ${WORKDIR}/$dts.tmp
       rm ${WORKDIR}/$dts.tmp
    done
}

do_install() {
    install -d ${D}/${sysconfdir}/overlays
    install -m 0755 ${WORKDIR}/*.dtbo ${D}${sysconfdir}/overlays
}

FILES:${PN} = "${sysconfdir}/overlays"

