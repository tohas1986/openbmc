DESCRIPTION = "CAuth updater"


inherit skeleton-rev

SRC_URI += "file://cauth-updater.service"
SRC_URI += "file://cauth-updater.timer"
SRC_URI += "file://cauth-updater.sh"
SRC_URI += "file://LICENSE"
SRCREV = 'a5e31b3ac0ee0df1a53783dc413e6c4c766b3299'

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=fa818a259cbed7ce8bc2a22d35a464fc"

S = "${WORKDIR}"

RDEPENDS:${PN} = "curl bash"

do_install () {
    install -d ${D}${systemd_unitdir}/system
    install -m 0644 ${WORKDIR}/cauth-updater.service ${D}${systemd_unitdir}/system/
    install -m 0644 ${WORKDIR}/cauth-updater.timer ${D}${systemd_unitdir}/system/
    install -d ${D}${sbindir}
    install -m 0755 ${S}/cauth-updater.sh ${D}${sbindir}/cauth-updater
}

pkg_postinst:${PN} () {
	if [ -n "$D" ]; then
		OPTS="--root=$D"
	fi

	if type systemctl >/dev/null 2>/dev/null; then
		systemctl $OPTS enable cauth-updater.timer
	fi
}

FILES:${PN} += "${sbindir}/cauth-updater ${systemd_unitdir}/system/cauth-updater.*"

