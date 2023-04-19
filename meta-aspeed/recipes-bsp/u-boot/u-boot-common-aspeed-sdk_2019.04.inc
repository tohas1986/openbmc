require conf/machine/distro/include/uboot-distrovars.inc

# HOMEPAGE = "https://github.com/AspeedTech-BMC/u-boot"
HOMEPAGE = "https://github.com/tohas1986/uboot-oy"
SECTION = "bootloaders"
DEPENDS += "flex-native bison-native xxd-native"

LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://Licenses/README;md5=30503fd321432fc713238f582193b78e"
PE = "1"

# We use the revision in order to avoid having to fetch it from the
# repo during parse

SRCREV = "67b0467218d61bde96162351d17d87cef04304d8"
# SRC_URI = "git://git@github.com/openbmc/u-boot.git;nobranch=1;protocol=https"
SRC_URI = "git://git@github.com/tohas1986/uboot-oy.git;nobranch=1;protocol=https"

SRC_URI:append:uboot-flash-32768 = " file://u-boot_flash_32M.cfg"
SRC_URI:append:uboot-flash-65536 = " file://u-boot_flash_64M.cfg"

S = "${WORKDIR}/git"
B = "${WORKDIR}/build"
do_configure[cleandirs] = "${B}"

# We don't have 'u-boot-initial-env' target in our u-boot version
UBOOT_INITIAL_ENV=""

PV = "v2019.04+git${SRCPV}"
#PV = "1.0+git${SRCPV}"
