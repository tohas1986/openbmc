SUMMARY = "Yandex LLDP forwarder"
DESCRIPTION = "Receive LLDP frame on one interface and send to another"


LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${YANDEXBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

S = "${WORKDIR}"
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI = "file://CMakeLists.txt\
           file://LICENSE \
           file://lldp-forwarder.c \
           "

inherit cmake pkgconfig
EXTRA_OECMAKE="-DYOCTO=1"

FILES:${PN}:append = " ${sbindir}/lldp-forwarder"
