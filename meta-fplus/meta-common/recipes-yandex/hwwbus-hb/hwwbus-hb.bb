SUMMARY = "Yandex HWWBUS heartbeat"
DESCRIPTION = "Send UDP heartbeat with own IPMI MAC address to HWWBUS endpoint"


LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${YANDEXBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

S = "${WORKDIR}"
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI = "file://CMakeLists.txt\
           file://LICENSE \
           file://hwwbus-hb.cpp \
           file://hwwbus-hb.hpp \
           file://hwwbus-hb.service \
           "

EXTRA_OECMAKE="-DYOCTO=1"

FILES:${PN}:append = " ${sbindir}/hwwbus-hb"

SYSTEMD_SERVICE:${PN} = "hwwbus-hb.service"

DEPENDS = "boost \
           phosphor-logging \
           systemd \
           sdbusplus \
           nlohmann-json \
           cli11 \
          "

inherit cmake pkgconfig systemd

do_install:append() {
    install -d ${D}/${systemd_unitdir}/system
    install -m 0644 ${WORKDIR}/hwwbus-hb.service ${D}/${systemd_unitdir}/system/
}

pkg_postinst:${PN} () {
    if [ -n "$D" ]; then
        OPTS="--root=$D"
    fi

    if type systemctl >/dev/null 2>/dev/null; then
        systemctl $OPTS enable hwwbus-hb.service
    fi
}

FILES:${PN} += "${sbindir}/hwwbus-hb ${systemd_unitdir}/system/hwwbus-hb.*"

