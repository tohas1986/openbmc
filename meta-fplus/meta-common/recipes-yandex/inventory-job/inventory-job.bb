DESCRIPTION = "Do jobs when inventory is ready"


inherit skeleton-rev

SRC_URI += "file://inventory-job.service"
SRC_URI += "file://inventory-job.sh"
SRC_URI += "file://LICENSE"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=a8328fd2a610bf4527feedcaa3ae3d14"

S = "${WORKDIR}"

RDEPENDS:${PN} = "bash"

do_install () {
    install -d ${D}${systemd_unitdir}/system
    install -m 0644 ${WORKDIR}/*.service ${D}${systemd_unitdir}/system/
    install -d ${D}${sbindir}
    install -m 0755 ${S}/inventory-job.sh ${D}${sbindir}/inventory-job
}

pkg_postinst:${PN} () {
	if [ -n "$D" ]; then
		OPTS="--root=$D"
	fi

	if type systemctl >/dev/null 2>/dev/null; then
		systemctl $OPTS enable inventory-job.service
	fi
}

FILES:${PN} += "${sbindir}/inventory-job ${systemd_unitdir}/system/inventory-job.service"
