DESCRIPTION = "USBNET startup service: Start USBNET in case it was enabled before reboot"


inherit skeleton-rev

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=a8328fd2a610bf4527feedcaa3ae3d14"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

RDEPENDS:${PN} = "bash"

S = "${WORKDIR}"

SRC_URI += "file://usbnet-start.service"
SRC_URI += "file://LICENSE"

do_install () {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/usbnet-start.service ${D}${systemd_system_unitdir}
}

FILES:${PN} += "${systemd_system_unitdir}/usbnet-start.service"

SYSTEMD_SERVICE:${PN} += "usbnet-start.service"

pkg_postinst:${PN} () {
	if [ -n "$D" ]; then
		OPTS="--root=$D"
	fi

	if type systemctl >/dev/null 2>/dev/null; then
		systemctl $OPTS enable usbnet-start.service
	fi
}
