FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

#
# Openyard AST2600 u-boot machine
#
SRC_URI:append = " \
                   file://0001-uboot-abr-disable.patch"



