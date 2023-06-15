DESCRIPTION = "Setting several BMC gpio to initial state"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit systemd

SRC_URI = " file://oy-gpio.sh \
            file://oy-gpio.service \
          "

S = "${WORKDIR}"

DEPENDS = "systemd"
RDEPENDS:${PN} = "bash"

SYSTEMD_SERVICE:${PN} = "oy-gpio.service"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${S}/oy-gpio.sh ${D}${bindir}/

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${S}/oy-gpio.service ${D}${systemd_system_unitdir}
}
