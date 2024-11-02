FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://breath.cfg \
    file://0001-Breath-dts.patch \
    "
