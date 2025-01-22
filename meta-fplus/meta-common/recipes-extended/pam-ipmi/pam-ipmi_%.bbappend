FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://ya_ipmi_pass"

do_install:append () {
        install -m 644 -pD ${WORKDIR}/ya_ipmi_pass ${D}${sysconfdir}/ipmi_pass
}

FILES:${PN} += "${sysconfdir}/ipmi_pass"
