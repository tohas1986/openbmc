FILESEXTRAPATHS:append := ":${THISDIR}/${PN}"

SRC_URI += "file://0001-Fix-vhub-port-to-p1.patch"
SRC_URI += "file://0006-Fix-missing-keboard-taps.patch"
SRC_URI += "file://0007-Disconnect-HID-before-connecting-it-back.patch"
