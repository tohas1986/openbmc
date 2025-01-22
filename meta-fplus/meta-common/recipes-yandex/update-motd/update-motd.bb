DESCRIPTION = "Update motd with IPv6 address service"


inherit skeleton-rev

SRC_URI += "file://update-motd.service"
SRC_URI += "file://update-motd.sh"
SRC_URI += "file://LICENSE"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=a8328fd2a610bf4527feedcaa3ae3d14"

S = "${WORKDIR}"

RDEPENDS:${PN} = "bash"

do_install () {
    install -d ${D}${systemd_unitdir}/system
    install -m 0644 ${WORKDIR}/*.service ${D}${systemd_unitdir}/system/
    install -d ${D}${sbindir}
    install -m 0755 ${S}/update-motd.sh ${D}${sbindir}/update-motd
}

pkg_postinst:${PN} () {
	if [ -n "$D" ]; then
		OPTS="--root=$D"
	fi

	if type systemctl >/dev/null 2>/dev/null; then
		systemctl $OPTS enable update-motd.service
	fi
}

FILES:${PN} += "${sbindir}/update-motd ${systemd_unitdir}/system/update-motd.service"
