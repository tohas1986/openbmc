SUMMARY = "Yandex OpenBmc Dobby sevice"
DESCRIPTION = "Collects sensors and other relevant data and generates big JSON"

#SRC_URI = "git://git@git.arc-vcs.yandex-team.ru/hwrnd-obmc-dobby;protocol=https;branch=trunk"
#SRCREV = "15c09b06a31a86a7e2451dafe9284680776579b9"

SRC_URI = "git://git@github.com:/tohas1986/obmc-dobby.git;protocol=ssh;branch=master"
SRCREV = "b12fba189e01f65bcb54ddcd63a0d008d122c5cc"


LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=e3fc50a88d0a364313df4b21ef20c29e"

SYSTEMD_SERVICE:${PN} = "dobby.service"

DEPENDS = "sdbusplus \
           phosphor-logging \
           nlohmann-json \
           cli11 \
           boost"

S = "${WORKDIR}/git"
inherit cmake pkgconfig systemd
