FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "\
    file://0001-Add-support-for-lan-settings.patch \
    file://0002-Add-smbios-read-commands.patch \
"
