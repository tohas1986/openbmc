DESCRIPTION = "Rebooter service - reboot board on memory consumption limit crossing"


inherit skeleton-rev

SRC_URI = "file://rebooter.service"
SRC_URI += "file://rebooter.sh"
SRCREV = '${AUTOREV}'

S = "${WORKDIR}"

RDEPENDS:${PN} = "luajit lua-curl"

do_install () {
    install -d ${D}${systemd_unitdir}/system
    install -m 0644 ${WORKDIR}/rebooter.service ${D}${systemd_unitdir}/system/rebooter.service
    install -d ${D}${sbindir}
    install -m 0755 ${S}/rebooter.sh ${D}${sbindir}/rebooter
}

pkg_postinst:${PN} () {
        if [ -n "$D" ]; then
                OPTS="--root=$D"
        fi

        if type systemctl >/dev/null 2>/dev/null; then
                systemctl $OPTS enable rebooter.service
        fi
}

FILES:${PN} += "${sbindir}/rebooter ${systemd_unitdir}/system/rebooter.service"
FILES:${PN} += " ${systemd_unitdir}/system/rebooter.service"

