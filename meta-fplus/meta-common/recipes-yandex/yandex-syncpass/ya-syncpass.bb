DESCRIPTION = "Sync ADMIN password service"

inherit skeleton-rev

SRC_URI += "file://syncpass.service"
SRC_URI += "file://syncpass.timer"
SRC_URI += "file://syncpass.sh"
SRC_URI += "file://checkpass.sh"
SRC_URI += "file://checkpass.timer"
SRC_URI += "file://checkpass.service"
SRC_URI += "file://LICENSE"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=a8328fd2a610bf4527feedcaa3ae3d14"

S = "${WORKDIR}"

RDEPENDS:${PN} = "curl bash"
SYSTEMD_SERVICE:${PN} = "syncpass.timer checkpass.timer"

do_install () {
    install -d ${D}${systemd_unitdir}/system
    install -m 0644 ${WORKDIR}/*.service ${D}${systemd_unitdir}/system/
    install -m 0644 ${WORKDIR}/*.timer ${D}${systemd_unitdir}/system/
    install -d ${D}${sbindir}
    install -m 0755 ${S}/syncpass.sh ${D}${sbindir}/syncpass
    install -m 0755 ${S}/checkpass.sh ${D}${sbindir}/checkpass
}

pkg_postinst:${PN} () {
    if [ -n "$D" ]; then
        OPTS="--root=$D"
    fi

    if type systemctl >/dev/null 2>/dev/null; then
        systemctl $OPTS enable syncpass.timer checkpass.timer
    fi
}

FILES:${PN} += "${sbindir}/syncpass ${sbindir}/checkpass ${systemd_unitdir}/system/syncpass.* ${systemd_unitdir}/system/checkpass.*"


