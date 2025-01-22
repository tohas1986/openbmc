SUMMARY = "OpenYard OCP gen3 host reboot at Power ON startup"
DESCRIPTION = "This service can reboot the host at Power ON startup"


LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${YANDEXBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

S = "${WORKDIR}"

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI = "file://host-reboot.sh \
           file://host-reboot.service"

SYSTEMD_SERVICE:${PN} = "host-reboot.service"

RDEPENDS:${PN} += "bash"

inherit pkgconfig systemd

do_install:append() {
    install -d ${D}/${systemd_unitdir}/system
    install -m 0644 ${WORKDIR}/host-reboot.service ${D}/${systemd_unitdir}/system

    install -d ${D}/usr/sbin/
    install -m 0755 ${WORKDIR}/host-reboot.sh ${D}/usr/sbin/
}

FILES:${PN} += "${sbindir}/host-reboot.sh ${systemd_unitdir}/system/host-reboot.*"