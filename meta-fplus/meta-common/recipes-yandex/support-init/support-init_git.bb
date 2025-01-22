SUMMARY = "Yandex OpenBmc template sevice"
DESCRIPTION = "In case you need to test something"

#SRC_URI = "git://git@git.arc-vcs.yandex-team.ru/hwrnd-support-init-service;protocol=https;branch=trunk"
#SRCREV = "b0355bd5e45ca6dd473149b55fbfe9013897c742"

SRC_URI = "git://github.com/tohas1986/support-init-service.git;protocol=https;branch=main"
SRCREV = "59179b5ae6d0325a51c7b7905eaf6855cbd0f74c"


LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${YANDEXBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

RDEPENDS:${PN} = "bash"

S = "${WORKDIR}"
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SYSTEMD_SERVICE:${PN} = "support-init.service"

SRC_URI += "file://my62-config.json"
SRC_URI += "file://mzb2-config.json"
SRC_URI += "file://som2600v1-config.json"
SRC_URI += "file://som2600v1-config.txt"
SRC_URI += "file://som2600v1-sec-config.json"
SRC_URI += "file://som2600v1-sec-config.txt"
SRC_URI += "file://finval_vr_settings.sh"

DEPENDS = "sdbusplus \
           phosphor-logging \
           boost \
           i2c-tools \
           cli11 \
           nlohmann-json"

S = "${WORKDIR}/git"
inherit cmake pkgconfig systemd

do_install:append() {
    install -d ${D}/usr/share/support-init
    install -m 0444 ${WORKDIR}/${MACHINE}-config.json ${D}/usr/share/support-init
    install -m 755 -pD ${WORKDIR}/finval_vr_settings.sh ${D}/usr/share/support-init/finval_vr_settings
}
