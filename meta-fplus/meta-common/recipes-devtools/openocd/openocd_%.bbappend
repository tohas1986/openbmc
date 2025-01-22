FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-Add-hardware-JTAG-driver.patch"

PACKAGECONFIG[jtag_driver] = "--enable-jtag_driver,--disable-jtag_driver"
PACKAGECONFIG:append = " jtag_driver"
