FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-Fix-compiler-errors.patch"

PACKAGECONFIG:remove = "nscd"
