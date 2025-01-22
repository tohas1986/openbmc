inherit obmc-phosphor-dbus-service
inherit obmc-phosphor-systemd

FILESEXTRAPATHS:append := "${THISDIR}/files:"
SUMMARY = "CPU Crashdump"
DESCRIPTION = "CPU utilities for dumping CPU Crashdump and registers over PECI"

DEPENDS = "boost cjson sdbusplus safec gtest libpeci"
inherit cmake pkgconfig

EXTRA_OECMAKE = "-DYOCTO_DEPENDENCIES=ON -DCRASHDUMP_BUILD_UT=OFF"

LICENSE = "Proprietary"
LIC_FILES_CHKSUM = "file://LICENSE;md5=43c09494f6b77f344027eea0a1c22830"

#SRC_URI = "git://git@git.arc-vcs.yandex-team.ru/hwrnd-crashdump;protocol=https;branch=trunk"
#SRCREV = "f3e498a6ff777dee7539d0c40c1a17cfa7b97e0b"

SRC_URI = "git://git@github.com:/tohas1986/crashdump.git;protocol=ssh;branch=master"
SRCREV = "0467f3534ca34bf485d4dbb3fdc9132b48f8a51e"

PV = "0.1+git${SRCPV}"

S = "${WORKDIR}/git"

SYSTEMD_SERVICE:${PN} += "com.intel.crashdump.service"

# linux-libc-headers guides this way to include custom uapi headers
CFLAGS:append = " -I ${STAGING_KERNEL_DIR}/include/uapi"
CFLAGS:append = " -I ${STAGING_KERNEL_DIR}/include"
CXXFLAGS:append = " -I ${STAGING_KERNEL_DIR}/include/uapi"
CXXFLAGS:append = " -I ${STAGING_KERNEL_DIR}/include"
do_configure[depends] += "virtual/kernel:do_shared_workdir"

FILES:${PN} += "/usr/share/*"
