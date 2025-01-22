DESCRIPTION = "Board-wide shell function library"


inherit skeleton-rev

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=a8328fd2a610bf4527feedcaa3ae3d14"

FILESEXTRAPATHS:append := "${THISDIR}/files:"

RDEPENDS:${PN} = "bash"

SRC_URI += "file://functions.sh"
SRC_URI += "file://usb-ctrl.sh"
SRC_URI += "file://usbnet.sh"
SRC_URI += "file://bmc-ready.sh"
SRC_URI += "file://bmc-ready.service"
SRC_URI += "file://LICENSE"
SRC_URI += "file://machine_${MACHINE}.sh"

SRC_URI += "file://fan_manual_control.sh"

S = "${WORKDIR}"

do_install () {
    install -m 644 -pD ${WORKDIR}/functions.sh ${D}${datadir}/openrack/functions
    install -m 644 -pD ${WORKDIR}/machine_${MACHINE}.sh ${D}${datadir}/openrack/machine
    install -m 755 -pD ${WORKDIR}/usb-ctrl.sh ${D}${sbindir}/usb-ctrl
    install -m 755 -pD ${WORKDIR}/usbnet.sh ${D}${sbindir}/usbnet
    install -m 755 -pD ${WORKDIR}/bmc-ready.sh ${D}${sbindir}/bmc-ready
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/bmc-ready.service ${D}${systemd_system_unitdir}
    install -m 755 -pD ${WORKDIR}/fan_manual_control.sh ${D}${datadir}/openrack/fan_manual_control
}

FILES:${PN} += "${datadir}/openrack/functions ${datadir}/openrack/machine ${sbindir}/bmc-ready ${sbindir}/usb-ctrl ${sbindir}/usbnet ${systemd_system_unitdir}/bmc-ready.service ${datadir}/openrack/fan_manual_control"

SYSTEMD_SERVICE:${PN} += "bmc-ready.service"

pkg_postinst:${PN} () {
        if [ -n "$D" ]; then
                OPTS="--root=$D"
        fi

        if type systemctl >/dev/null 2>/dev/null; then
                systemctl $OPTS enable bmc-ready.service
        fi
}
