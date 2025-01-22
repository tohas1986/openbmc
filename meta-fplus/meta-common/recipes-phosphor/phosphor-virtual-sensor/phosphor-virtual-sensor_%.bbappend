FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " file://${MACHINE}_sensor_config.json"

do_install:append() {

    install -d ${D}/usr/share/phosphor-virtual-sensor

    install -m 0644 -D ${WORKDIR}/${MACHINE}_sensor_config.json \
                   ${D}/usr/share/phosphor-virtual-sensor/virtual_sensor_config.json
}
SRC_URI += "file://0001-Revert-Ed-Tanos-breaking-commit.patch"

