SUMMARY = "Yandex RMT Parser"
DESCRIPTION = "This service can parse obmc-console.log in order to get Worst case margin resoults of DRAM signal validation"


LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${YANDEXBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

S = "${WORKDIR}"
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI = "file://CMakeLists.txt \
           file://LICENSE \
           file://rmtparser.cpp \
           file://rmtparser.hpp \
           file://rmtparser.service "

inherit cmake pkgconfig systemd
DEPENDS = "boost \
           phosphor-logging \
           systemd \
           sdbusplus \
           nlohmann-json \
           cli11"

EXTRA_OECMAKE="-DYOCTO=1"

SYSTEMD_SERVICE:${PN} += "rmtparser.service"

do_install:append() {
    install -d ${D}/${systemd_unitdir}/system
    install -m 0644 ${WORKDIR}/rmtparser.service ${D}/${systemd_unitdir}/system
}
