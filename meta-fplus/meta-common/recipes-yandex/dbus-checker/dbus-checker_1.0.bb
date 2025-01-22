DESCRIPTION = "D-Bus hang checker"


inherit skeleton-rev

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=a8328fd2a610bf4527feedcaa3ae3d14"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

RDEPENDS:${PN} = "bash"

S = "${WORKDIR}"

SRC_URI += "file://dbus-checker.sh"
SRC_URI += "file://dbus-checker.service"
SRC_URI += "file://LICENSE"

do_install () {
    install -m 755 -pD ${WORKDIR}/dbus-checker.sh ${D}${datadir}/openrack/dbus-checker

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/dbus-checker.service ${D}${systemd_system_unitdir}
}

FILES:${PN} += "${datadir}/openrack/dbus-checker ${systemd_system_unitdir}/dbus-checker.service"

SYSTEMD_SERVICE:${PN} += "dbus-checker.service"

pkg_postinst:${PN} () {
	if [ -n "$D" ]; then
		OPTS="--root=$D"
	fi

	if type systemctl >/dev/null 2>/dev/null; then
		systemctl $OPTS enable dbus-checker.service
	fi
}
