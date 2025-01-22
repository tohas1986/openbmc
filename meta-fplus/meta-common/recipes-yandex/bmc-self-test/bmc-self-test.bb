SUMMARY = "BMC self test service"
DESCRIPTION = "Yandex BMC selftest service for EINE usage"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${YANDEXBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

RDEPENDS:${PN} = "bash"

SRC_URI += "file://bmc-self-test.service \
            file://bmc-self-test.sh \
            file://bmc-self-test-functions.sh \
           "

SYSTEMD_SERVICE:${PN} = "bmc-self-test.service"
SYSTEMD_AUTO_ENABLE_${PN} = "disable"

S = "${WORKDIR}"

do_install:append() {
    install -d ${D}/usr/share/bmc-self-test

    install -d ${D}/lib/systemd/system/
    install -m 0644 ${S}/bmc-self-test.service ${D}/lib/systemd/system/

    install -d ${D}${sbindir}
    install -m 0755 ${S}/bmc-self-test.sh ${D}${sbindir}/bmc-self-test
    install -m 0755 ${S}/bmc-self-test-functions.sh ${D}${sbindir}/bmc-self-test-functions
}

#pkg_postinst:${PN} () {
#        if [ -n "$D" ]; then
#                OPTS="--root=$D"
#        fi
#
#        if type systemctl >/dev/null 2>/dev/null; then
#                systemctl $OPTS enable bmc-self-test.service
#        fi
#}

FILES:${PN} += "${sbindir}/bmc-self-test ${systemd_unitdir}/system/bmc-self-test.service"
FILES:${PN} += " ${systemd_unitdir}/system/bmc-self-test.service"
