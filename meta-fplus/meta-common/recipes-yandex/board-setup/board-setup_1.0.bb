DESCRIPTION = "Board setup: load overlay, copy settings between EEPROM and U-Boot env, etc"


inherit skeleton-rev

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=a8328fd2a610bf4527feedcaa3ae3d14"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:${THISDIR}/files/custom:"

RDEPENDS:${PN} = "bash"

S = "${WORKDIR}"

SRC_URI += "file://board-setup.sh"
SRC_URI += "file://board-setup.service"
SRC_URI += "file://empty_fru.bin"
SRC_URI += "file://LICENSE"

# Re-driver settings
SRC_URI += "file://58_settings.set"
SRC_URI += "file://5a_settings.set"
SRC_URI += "file://5c_settings.set"
SRC_URI += "file://5e_settings.set"



# Per machine custom startup scripts
SRC_URI += "file://unknown.sh "
SRC_URI:append:${MACHINE} = "file://${MACHINE}.sh "

do_install () {
    install -m 755 -pD ${WORKDIR}/board-setup.sh ${D}${datadir}/openrack/board-setup
    install -m 755 -pD ${WORKDIR}/empty_fru.bin ${D}${datadir}/openrack/empty_fru.bin

    install -m 755 -pD ${WORKDIR}/unknown.sh ${D}${datadir}/openrack/custom/unknown
    install -m 755 -pD ${WORKDIR}/${MACHINE}.sh ${D}${datadir}/openrack/custom/${MACHINE}

    install -m 755 -pD ${WORKDIR}/58_settings.set ${D}${datadir}/openrack/58_settings.set
    install -m 755 -pD ${WORKDIR}/5a_settings.set ${D}${datadir}/openrack/5a_settings.set
    install -m 755 -pD ${WORKDIR}/5c_settings.set ${D}${datadir}/openrack/5c_settings.set
    install -m 755 -pD ${WORKDIR}/5e_settings.set ${D}${datadir}/openrack/5e_settings.set

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/board-setup.service ${D}${systemd_system_unitdir}
}

FILES:${PN} += "${datadir}/openrack/board-setup ${datadir}/openrack/custom/* ${datadir}/openrack/empty_fru.bin ${systemd_system_unitdir}/board-setup.service"
FILES:${PN} += "${datadir}/openrack/58_settings.set  ${datadir}/openrack/5a_settings.set ${datadir}/openrack/5c_settings.set ${datadir}/openrack/5e_settings.set"

SYSTEMD_SERVICE:${PN} += "board-setup.service"

pkg_postinst:${PN} () {
	if [ -n "$D" ]; then
		OPTS="--root=$D"
	fi

	if type systemctl >/dev/null 2>/dev/null; then
		systemctl $OPTS enable board-setup.service
	fi
}
