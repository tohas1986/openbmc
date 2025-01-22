SUMMARY = "DMI (Desktop Management Interface) table related utilities with Yandex add-ons"
HOMEPAGE = "http://www.nongnu.org/dmidecode/"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://LICENSE;md5=b234ee4d69f5fce4486a80fdaf4a4263"

#SRC_URI = "git://git@git.arc-vcs.yandex-team.ru/hwrnd-obmc-dmidecode;protocol=https;branch=trunk"
#SRCREV = "e9699c7ebb8597755aa92717a7c03eeb7dcb6e32"

SRC_URI = "git://git@github.com:/tohas1986/dmidecode.git;protocol=ssh;branch=master"
SRCREV = "75b7a91ebe5306bc26119daeaeecc85286e5a2aa"


S = "${WORKDIR}/${PV}"

COMPATIBLE_HOST = "(i.86|x86_64|aarch64|arm|powerpc|powerpc64).*-linux"

EXTRA_OEMAKE = "-e MAKEFLAGS="

# The upstream buildsystem uses 'docdir' as the path where it puts AUTHORS,
# README, etc, but we don't want those in the root of our docdir.
docdir .= "/${BPN}"

do_install() {
        oe_runmake DESTDIR="${D}" install
}

SRC_URI[md5sum] = "9cc2e27e74ade740a25b1aaf0412461b"
SRC_URI[sha256sum] = "077006fa2da0d06d6383728112f2edef9684e9c8da56752e97cd45a11f838edd"

