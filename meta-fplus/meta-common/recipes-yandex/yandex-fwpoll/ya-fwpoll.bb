DESCRIPTION = "Firmware poller service"


inherit skeleton-rev

SRC_URI += "file://fwpoll.service"
SRC_URI += "file://fwpoll.timer"
SRC_URI += "file://fwpoll.sh"
SRC_URI += "file://fwupgrade.sh"
SRC_URI += "file://JSON.sh"
SRC_URI += "file://LICENSE"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=a8328fd2a610bf4527feedcaa3ae3d14"

S = "${WORKDIR}"

RDEPENDS:${PN} = "curl bash"

do_install () {
    install -d ${D}${systemd_unitdir}/system
    install -d ${D}${datadir}/openrack
    install -m 0644 ${WORKDIR}/fwpoll.service ${D}${systemd_unitdir}/system/
    install -m 0644 ${WORKDIR}/fwpoll.timer ${D}${systemd_unitdir}/system/
    install -d ${D}${sbindir}
    install -m 0755 ${S}/fwpoll.sh ${D}${sbindir}/fwpoll
    install -m 0755 ${S}/fwupgrade.sh ${D}${sbindir}/fwupgrade
    install -m 0755 ${S}/JSON.sh ${D}${datadir}/openrack/
}

pkg_postinst:${PN} () {
	if [ -n "$D" ]; then
		OPTS="--root=$D"
	fi

	if type systemctl >/dev/null 2>/dev/null; then
		systemctl $OPTS enable fwpoll.timer
	fi
}

FILES:${PN} += "${sbindir}/fwpoll ${sbindir}/fwupgrade ${datadir}/openrack/JSON.sh ${systemd_unitdir}/system/fwpoll.*"
