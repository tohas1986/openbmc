FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-Fixup-Non-matching-signing-type-error.patch"
SRC_URI += "file://dropbear.service"
SRC_URI += "file://dropbear.prod"
SRC_URI += "file://dropbear.factory"

inherit systemd

SYSTEMD_SERVICE:${PN} = "dropbear.service"
SYSTEMD_SERVICE:${PN}:remove = "dropbear.socket"
SYSTEMD_AUTO_ENABLE:${PN} = "disable"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/dropbear.service ${D}${systemd_system_unitdir}
    find ${D} -name "dropbear.socket" -exec rm -rf {} \; || :
    find ${D} -name "dropbear@.service" -exec rm -rf {} \; || :
    find ${D} -name "dropbearkey.service" -exec rm -rf {} \; || :
    install -d ${D}/rwfs/etc/default/
    install -m 0644 ${WORKDIR}/dropbear.factory ${D}/rwfs/etc/default/dropbear
    install -d ${D}/etc/default/
    install -m 0644 ${WORKDIR}/dropbear.prod ${D}/etc/default/dropbear
}

FILES:${PN} += "/etc/default/dropbear /rwfs/etc/default/dropbear"

#pkg_postinst:${PN} () {
#    if [ -n "$D" ]; then
#        OPTS="--root=$D"
#    fi
#
#    if type systemctl >/dev/null 2>/dev/null; then
#        systemctl $OPTS enable dropbear.service
#    fi
#}
