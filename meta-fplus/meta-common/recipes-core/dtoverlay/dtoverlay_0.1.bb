DESCRIPTION = "DTS overlay & merge utils"
LICENSE = "PD"
LIC_FILES_CHKSUM = "file://dtoverlay_main.c;beginline=2;endline=25;md5=96679a98d4f3d5a5be58f1e7652d98fa"

SRC_URI = "git://github.com/tohas1986/dtoverlay.git;protocol=https;branch=main"
SRCREV = "e4e7be0f6d68df082862c4d8cedde0ca00785378"

#SRC_URI = "git://git@git.arc-vcs.yandex-team.ru/hwrnd-dtoverlay;protocol=https;branch=trunk"
SRC_URI += "file://001-cmake-include.patch"
#SRCREV = "5eb36b3c4a99b16573b3ec05f21f2b638066e2f5"

S = "${WORKDIR}/git"

inherit cmake pkgconfig

FILES:${PN} = "${bindir}"
