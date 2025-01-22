SUMMARY = "Chassis Power Control service for Intel based platforms"
DESCRIPTION = "Chassis Power Control service for Intel based platforms"

#SRC_URI = "git://git@git.arc-vcs.yandex-team.ru/hwrnd-x86-power-control;protocol=https;branch=trunk"
#SRCREV = "8292f4f0d1a0dda71d0a9a35b0130d69bf98d2ce"

SRC_URI = "git://git@github.com:/tohas1986/x86-power.git;protocol=ssh;branch=master"
SRCREV = "400f7fc6615dd3d024442e46c13239a280ebf6db"

PV = "1.0+git${SRCREV}"

S = "${WORKDIR}/git"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=86d3f3a95c324c9479bd8986968f4327"

inherit cmake pkgconfig systemd
inherit obmc-phosphor-dbus-service

SYSTEMD_SERVICE:x86-power-control-yandex = "xyz.openbmc_project.Chassis.Control.Power.service \
                         chassis-system-reset.service \
                         chassis-system-reset.target"

DEPENDS += " \
    boost \
    i2c-tools \
    libgpiod \
    nlohmann-json \
    sdbusplus \
    phosphor-logging \
  "

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://power-config-host0_default.json"
SRC_URI += "file://power-config-host0_${MACHINE}.json"

do_install:append(){
     install -d ${D}/usr/share/x86-power-control
     [ -f "${WORKDIR}/power-config-host0_${MACHINE}.json" ] && install -m 0444 ${WORKDIR}/power-config-host0_${MACHINE}.json ${D}/usr/share/x86-power-control/power-config-host0.json || \
        install -m 0444 ${WORKDIR}/power-config-host0_default.json ${D}/usr/share/x86-power-control/power-config-host0.json
}

FILES:${PN} += "usr/share/x86-power-control/power-config-host0.json"
