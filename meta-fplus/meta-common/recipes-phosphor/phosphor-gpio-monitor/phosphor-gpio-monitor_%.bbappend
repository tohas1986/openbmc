FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://jbog_gpio.json"
SRC_URI += "file://0001-Support-GPIO-interrupt.patch"
SRC_URI += "file://0002-Change-multi-gpio-service-options.patch"

SRC_URI += "file://SetPowerGoodPropertyOff.service \
            file://SetPowerGoodPropertyOn.service \
            file://setPowerProperty.sh \
            "
SRC_URI += "file://SetGPUPowerGoodPropertyOff.service \
            file://SetGPUPowerGoodPropertyOn.service \
            file://setGPUPowerProperty.sh \
            "
SRC_URI += "file://SetPowerGoodPropertyCycle.service \
             file://setGPUPowerPropertyCycle.sh \
            "
SRC_URI += "file://setHostBmcArbitration.service \
             file://setHostBmcArbitration.sh \
            "
FILES:${PN}-monitor += "${datadir}/phosphor-gpio-monitor/phosphor-multi-gpio-monitor.json"
FILES:${PN}-monitor += "${sbindir}/setPowerProperty.sh"
FILES:${PN}-monitor += "${sbindir}/setGPUPowerProperty.sh"
FILES:${PN}-monitor += "${sbindir}/setGPUPowerPropertyCycle.sh"
FILES:${PN}-monitor += "${sbindir}/setHostBmcArbitration.sh"

SYSTEMD_SERVICE:${PN}-monitor += "SetPowerGoodPropertyOff.service"
SYSTEMD_SERVICE:${PN}-monitor += "SetPowerGoodPropertyOn.service"

SYSTEMD_SERVICE:${PN}-monitor += "SetGPUPowerGoodPropertyOff.service"
SYSTEMD_SERVICE:${PN}-monitor += "SetGPUPowerGoodPropertyOn.service"

SYSTEMD_SERVICE:${PN}-monitor += "SetPowerGoodPropertyCycle.service"

SYSTEMD_SERVICE:${PN}-monitor += "setHostBmcArbitration.service"

FILES:${PN}-monitor += "${sbindir}/SetPowerGoodPropertyOff.service ${systemd_unitdir}/system/SetPowerGoodPropertyOff.service"
FILES:${PN}-monitor += "${sbindir}/SetPowerGoodPropertyOn.service ${systemd_unitdir}/system/SetPowerGoodPropertyOn.service"

FILES:${PN}-monitor += "${sbindir}/SetGPUPowerGoodPropertyOff.service ${systemd_unitdir}/system/SetGPUPowerGoodPropertyOff.service"
FILES:${PN}-monitor += "${sbindir}/SetGPUPowerGoodPropertyOn.service ${systemd_unitdir}/system/SetGPUPowerGoodPropertyOn.service"

FILES:${PN}-monitor += "${sbindir}/SetPowerGoodPropertyCycle.service ${systemd_unitdir}/system/SetPowerGoodPropertyCycle.service"

FILES:${PN}-monitor += "${sbindir}/setHostBmcArbitration.service ${systemd_unitdir}/system/setHostBmcArbitration.service"

do_install:append(){
    install -m 0644 ${WORKDIR}/SetPowerGoodPropertyOff.service ${D}${systemd_unitdir}/system/
    install -m 0644 ${WORKDIR}/SetPowerGoodPropertyOn.service ${D}${systemd_unitdir}/system/

    install -m 0644 ${WORKDIR}/SetGPUPowerGoodPropertyOff.service ${D}${systemd_unitdir}/system/
    install -m 0644 ${WORKDIR}/SetGPUPowerGoodPropertyOn.service ${D}${systemd_unitdir}/system/

    install -m 0644 ${WORKDIR}/SetPowerGoodPropertyCycle.service ${D}${systemd_unitdir}/system/

    install -m 0644 ${WORKDIR}/setHostBmcArbitration.service ${D}${systemd_unitdir}/system/

    install -d ${D}/usr/share/phosphor-gpio-monitor

    install -m 0444 ${WORKDIR}/*.json ${D}/usr/share/phosphor-gpio-monitor/phosphor-multi-gpio-monitor.json

    install -d ${D}/usr/sbin

    install -m 0755 ${WORKDIR}/setPowerProperty.sh ${D}/${sbindir}/
    install -m 0755 ${WORKDIR}/setGPUPowerProperty.sh ${D}/${sbindir}/
    install -m 0755 ${WORKDIR}/setGPUPowerPropertyCycle.sh ${D}/${sbindir}/
    install -m 0755 ${WORKDIR}/setHostBmcArbitration.sh ${D}/${sbindir}/
}
