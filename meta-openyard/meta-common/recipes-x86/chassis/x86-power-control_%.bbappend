FILESEXTRAPATHS:prepend:dev2600 := "${THISDIR}/${PN}:"
SRC_URI:append:dev2600 = " file://power-config-host0.json"

SRC_URI += "\
    file://0001-x86power-control.patch \
"

EXTRA_OEMESON:openyard += "-Duse-plt-rst=enabled"
EXTRA_OEMESON:openyard += "-Dignore-soft-resets-during-post=enabled"
EXTRA_OEMESON:openyard += "-Duse-acboot=enabled"

do_install:append:dev2600() {
    install -m 0755 -d ${D}/${datadir}/${BPN}
    install -m 0644 ${WORKDIR}/power-config-host0.json ${D}${datadir}/${BPN}
}
