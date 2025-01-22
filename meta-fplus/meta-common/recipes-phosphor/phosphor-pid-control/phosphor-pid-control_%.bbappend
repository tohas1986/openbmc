FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

# Fix error with derivativeCoeff"
# FIXME To be removed on the next sync with upstream
SRCREV = "9f9a06aa9c4d49749c2d94164ed00c6ef706e04f"

SRC_URI += "file://phosphor-pid-control.service"
SRC_URI += "file://config-${MACHINE}.json"

SRC_URI += "file://ai1_fan_data.txt"
SRC_URI += "file://my62_fan_data.txt"
SRC_URI += "file://mzb2_fan_data.txt"
SRC_URI += "file://gd1_fan_data.txt"
SRC_URI += "file://gd2_fan_data.txt"
SRC_URI += "file://my81_fan_data.txt"
SRC_URI += "file://mz81_fan_data.txt"
SRC_URI += "file://g1_fan_data.txt"
SRC_URI += "file://fan_controller_pwd.txt"

SRC_URI += "file://0001-Change-stepwise-to-use-linear-Interpolation.patch"
SRC_URI += "file://0002-Add-setPwm-command-subcommand-3.patch"

SRC_URI += "file://0003-Disable-log-buffering.patch"
SRC_URI += "file://0004-Add-set-NVME-Cooling-Setpoint-method.patch"

SRC_URI += "file://0006-Make-failsafe-dependant-on-power-state-and-config.patch"
SRC_URI += "file://0007-Add-separate-fancontrol-functions.patch"

SYSTEMD_SERVICE:${PN} = "phosphor-pid-control.service"

FILES:${PN}:append = " ${datadir}/swampd/config.json ${datadir}/swampd/*fan_data.txt ${datadir}/swampd/fan_controller_pwd.txt"

do_install:append(){
    install -d ${D}${datadir}/swampd
    install -m 0644 -D ${WORKDIR}/config-${MACHINE}.json \
        ${D}${datadir}/swampd/config.json
    install -m 0644 -D ${WORKDIR}/ai1_fan_data.txt \
        ${D}${datadir}/swampd/ai1_fan_data.txt
    install -m 0644 -D ${WORKDIR}/my62_fan_data.txt \
        ${D}${datadir}/swampd/my62_fan_data.txt
    install -m 0644 -D ${WORKDIR}/mzb2_fan_data.txt \
        ${D}${datadir}/swampd/mzb2_fan_data.txt
    install -m 0644 -D ${WORKDIR}/gd1_fan_data.txt \
        ${D}${datadir}/swampd/gd1_fan_data.txt
    install -m 0644 -D ${WORKDIR}/gd2_fan_data.txt \
        ${D}${datadir}/swampd/gd2_fan_data.txt
    install -m 0644 -D ${WORKDIR}/my81_fan_data.txt \
        ${D}${datadir}/swampd/my81_fan_data.txt
    install -m 0644 -D ${WORKDIR}/mz81_fan_data.txt \
        ${D}${datadir}/swampd/mz81_fan_data.txt
    install -m 0644 -D ${WORKDIR}/g1_fan_data.txt \
        ${D}${datadir}/swampd/g1_fan_data.txt
    install -m 0644 -D ${WORKDIR}/fan_controller_pwd.txt \
        ${D}${datadir}/swampd/fan_controller_pwd.txt
    install -d ${D}${systemd_unitdir}/system/
    install -m 0644 ${WORKDIR}/phosphor-pid-control.service \
        ${D}${systemd_unitdir}/system
}
