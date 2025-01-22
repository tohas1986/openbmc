DESCRIPTION = "Startup saver: emergency reboot in case of dbus stall"


inherit skeleton-rev

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=a8328fd2a610bf4527feedcaa3ae3d14"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

RDEPENDS:${PN} = "bash"

S = "${WORKDIR}"

SRC_URI += "file://startup-saver.sh"
SRC_URI += "file://startup-saver.service"
SRC_URI += "file://LICENSE"

do_install () {
    install -m 755 -pD ${WORKDIR}/startup-saver.sh ${D}${datadir}/openrack/startup-saver

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/startup-saver.service ${D}${systemd_system_unitdir}
}

FILES:${PN} += "${datadir}/openrack/startup-saver ${systemd_system_unitdir}/startup-saver.service"

SYSTEMD_SERVICE:${PN} += "startup-saver.service"

pkg_postinst:${PN} () {
	if [ -n "$D" ]; then
		OPTS="--root=$D"
	fi

	if type systemctl >/dev/null 2>/dev/null; then
		systemctl $OPTS enable startup-saver.service
	fi
}
