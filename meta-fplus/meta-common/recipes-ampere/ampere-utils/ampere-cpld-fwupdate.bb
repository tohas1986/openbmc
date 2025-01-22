SUMMARY = "Ampere Computing LLC CPLD Firmware Update"
DESCRIPTION = "Application to support CPLD Firmware Update on Ampere platforms"
PR = "r0"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"
inherit meson pkgconfig systemd
DEPENDS = "boost sdbusplus systemd"

S = "${WORKDIR}/git/ampere-cpld-fwupdate"
SRC_URI = "git://github.com/ampere-openbmc/ampere-misc;protocol=https;branch=ampere"
SRCREV= "73b89517baa851749c6349296c3b8df7fccfae17"
