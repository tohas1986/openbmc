DESCRIPTION = "Save RWFS to persistent partition"


inherit skeleton-rev

SRC_URI += "file://config-saver.service"
SRC_URI += "file://config-saver.sh"
SRC_URI += "file://LICENSE"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=a8328fd2a610bf4527feedcaa3ae3d14"

S = "${WORKDIR}"

RDEPENDS:${PN} = "bash"

do_install () {
    install -d ${D}${systemd_unitdir}/system
    install -m 0644 ${WORKDIR}/*.service ${D}${systemd_unitdir}/system/
    install -d ${D}${sbindir}
    install -m 0755 ${S}/config-saver.sh ${D}${sbindir}/config-saver
}

pkg_postinst:${PN} () {
	if [ -n "$D" ]; then
		OPTS="--root=$D"
	fi

	if type systemctl >/dev/null 2>/dev/null; then
		systemctl $OPTS enable config-saver.service
	fi
}

FILES:${PN} += "${sbindir}/config-saver ${systemd_unitdir}/system/config-saver.service"
