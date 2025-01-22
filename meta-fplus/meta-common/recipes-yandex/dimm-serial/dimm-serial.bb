SUMMARY = "OpenYard DIMM big serial Service"
DESCRIPTION = "This service can re-set long DIMM S/N for some vendors"


LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${YANDEXBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

S = "${WORKDIR}"

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI = "file://CMakeLists.txt\
           file://LICENSE \
           file://dimmserial.cpp \
           file://dimmserial.hpp \
           file://dimmserial_start.sh \
           file://dimmserial.service"

# Certificates
SRC_URI += "file://CA-cert.pem"
SRC_URI += "file://cert.pem"

SYSTEMD_SERVICE:${PN} = "dimmserial.service"

DEPENDS = "boost \
           phosphor-logging \
           systemd \
           sdbusplus \
           i2c-tools \
           nlohmann-json \
           cli11 \
          "

RDEPENDS:${PN} += "bash"

inherit cmake pkgconfig systemd

do_install:append() {
    install -d ${D}/${systemd_unitdir}/system
    install -m 0644 ${WORKDIR}/dimmserial.service ${D}/${systemd_unitdir}/system

    install -d ${D}/usr/sbin/
    install -m 0755 ${WORKDIR}/dimmserial_start.sh ${D}/usr/sbin/
    install -m 0755 ${WORKDIR}/CA-cert.pem ${D}/usr/sbin/
    install -m 0755 ${WORKDIR}/cert.pem ${D}/usr/sbin/
}

FILES:${PN} += "${sbindir}/dimmserial ${sbindir}/dimmserial_start.sh ${sbindir}/CA-cert.pem ${sbindir}/cert.pem ${systemd_unitdir}/system/dimmserial.*"
