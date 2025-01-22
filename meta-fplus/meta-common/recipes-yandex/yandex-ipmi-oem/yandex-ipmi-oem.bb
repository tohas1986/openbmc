SUMMARY = "Yandex OEM IPMI commands"
DESCRIPTION = "Yandex OEM Commands"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${YANDEXBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

S = "${WORKDIR}"
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI = "file://CMakeLists.txt\
           file://LICENSE \
           file://oemcmd.cpp \
           file://oemcmd.hpp \
           file://file.hpp "

DEPENDS = "boost phosphor-ipmi-host phosphor-logging systemd "
DEPENDS += "libgpiod libpeci i2c-tools"

inherit cmake pkgconfig obmc-phosphor-ipmiprovider-symlink
EXTRA_OECMAKE="-DYOCTO=1"


LIBRARY_NAMES = "libyndxoemcmds.so"

FILES:${PN}:append = " ${libdir}/ipmid-providers/lib*${SOLIBS}"
FILES:${PN}:append = " ${libdir}/host-ipmid/lib*${SOLIBS}"
FILES:${PN}:append = " ${libdir}/net-ipmid/lib*${SOLIBS}"
FILES:${PN}-dev:append = " ${libdir}/ipmid-providers/lib*${SOLIBSDEV}"

#linux-libc-headers guides this way to include custom uapi headers
CFLAGS:append = " -I ${STAGING_KERNEL_DIR}/include/uapi"
CFLAGS:append = " -I ${STAGING_KERNEL_DIR}/include"
CXXFLAGS:append = " -I ${STAGING_KERNEL_DIR}/include/uapi"
CXXFLAGS:append = " -I ${STAGING_KERNEL_DIR}/include"
do_configure[depends] += "virtual/kernel:do_shared_workdir"
