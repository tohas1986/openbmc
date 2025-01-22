FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

FILES:${PN} += "${sysconfdir}/nbd-proxy/state"

SRC_URI += "file://0001-Add-media-type-USB-HDD-CDROM.patch"
SRC_URI += "file://0002-Use-etc-nbd-proxy-state-yandex-instead-of-delivered.patch"
SRC_URI += "file://state_hook-yandex"

do_install:append() {
    install -d ${D}${sysconfdir}/nbd-proxy/
    install -m 0755 ${WORKDIR}/state_hook-yandex ${D}${sysconfdir}/nbd-proxy/state-yandex
}
