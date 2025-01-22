SUMMARY = "Yandex NVME cooler"
DESCRIPTION = "This service can set NVME cooling setpoint"


LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${YANDEXBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

S = "${WORKDIR}"

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI = "file://CMakeLists.txt\
           file://LICENSE \
           file://nvmecooler.cpp \
           file://nvmecooler.hpp \
           file://nvmecooler.service \
           file://nvmecooler.timer "

SRC_URI += "file://nvme-pn-table.json"
SRC_URI += "file://smbus.hpp"
SRC_URI += "file://i2c.h"
SRC_URI += "file://smbus.cpp"

SYSTEMD_SERVICE:${PN} = "nvmecooler.service nvmecooler.timer"

DEPENDS = "boost \
           phosphor-logging \
           systemd \
           sdbusplus \
           i2c-tools \
           nlohmann-json \
           cli11 \
          "

inherit cmake pkgconfig systemd

do_install:append() {
    install -d ${D}/usr/share/nvme-cooler
    install -m 0444 ${WORKDIR}/nvme-pn-table.json ${D}/usr/share/nvme-cooler
    install -d ${D}/${systemd_unitdir}/system
    install -m 0644 ${WORKDIR}/nvmecooler.service ${D}/${systemd_unitdir}/system
    install -m 0644 ${WORKDIR}/nvmecooler.timer ${D}/${systemd_unitdir}/system
}

pkg_postinst:${PN} () {
    if [ -n "$D" ]; then
        OPTS="--root=$D"
    fi

    if type systemctl >/dev/null 2>/dev/null; then
        systemctl $OPTS enable nvmecooler.timer
    fi
}

FILES:${PN} += "${sbindir}/nvmecooler ${systemd_unitdir}/system/nvmecooler.*"
