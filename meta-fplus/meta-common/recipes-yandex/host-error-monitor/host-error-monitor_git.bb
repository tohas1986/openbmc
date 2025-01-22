LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=7becf906c8f8d03c237bad13bc3dac53"
inherit cmake pkgconfig systemd

#SRC_URI = "git://git@git.arc-vcs.yandex-team.ru/hwrnd-host-error-monitor;protocol=https;branch=trunk"
#SRCREV = "ddf4c1e000bb146d931ded034b7be0c00f8fa0f2"

SRC_URI = "git://git@github.com:/tohas1986/host-error-monitor.git;protocol=ssh;branch=master"
SRCREV = "54c9c2f9e551d57f690d7c1692d2d846302b09cc"

DEPENDS = "boost sdbusplus libgpiod libpeci nlohmann-json cli11"

SRC_URI += "file://host-error-monitor_my81.json"
SRC_URI += "file://host-error-monitor_mzb2.json"
SRC_URI += "file://host-error-monitor_my62.json"
SRC_URI += "file://host-error-monitor_mz81.json"
SRC_URI += "file://host-error-monitor_ai1.json"
SRC_URI += "file://host-error-monitor_fp01.json"

S = "${WORKDIR}/git"

SYSTEMD_SERVICE:${PN} += "host-error-monitor.service"

EXTRA_OECMAKE = "-DYOCTO=1"

do_install:append(){
    install -d ${D}/usr/share/host-error-monitor
    [ -f "${WORKDIR}/host-error-monitor_${MACHINE}.json" ] && install -m 0444 ${WORKDIR}/host-error-monitor_${MACHINE}.json ${D}/usr/share/host-error-monitor/host-error-monitor.json || \
    install -m 0444 ${WORKDIR}/host-error-monitor_default.json ${D}/usr/share/host-error-monitor/host-error-monitor.json
}
