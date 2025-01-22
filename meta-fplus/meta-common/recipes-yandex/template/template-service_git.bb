SUMMARY = "Yandex OpenBmc template sevice"
DESCRIPTION = "In case you need to test something"

#SRC_URI = "git://git@git.arc-vcs.yandex-team.ru/hwrnd-ya-openbmc-template;protocol=https;branch=trunk"
#SRCREV = "5e4e124db016ab00a6a0aae3d687903462a97b86"
SRC_URI = "git://github.com/tohas1986/template.git;protocol=https;branch=main"
SRCREV = "8811d76ac2be278a5b3a8758f37c93f474ccbfd8"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=e3fc50a88d0a364313df4b21ef20c29e"

SYSTEMD_SERVICE:${PN} = "template-service.service"

DEPENDS = "sdbusplus \
           phosphor-logging \
           boost"

S = "${WORKDIR}/git"
inherit cmake pkgconfig systemd
