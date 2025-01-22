FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://led-group-config.${MACHINE}"

EXTRA_OECONF += "--enable-use-json --disable-use-lamp-test"

DEPENDS += "phosphor-dbus-interfaces"

do_install:append() {
        install -d ${D}${datadir}/phosphor-led-manager/
        install -m 0644 ${WORKDIR}/led-group-config.${MACHINE} ${D}${datadir}/phosphor-led-manager/led-group-config.json
        rm -rf {D}${datadir}/phosphor-led-manager/ibm*
}
