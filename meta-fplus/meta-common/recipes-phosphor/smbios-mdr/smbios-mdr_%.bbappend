FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0002-Add-OEM-NIC-parser.patch "
SRC_URI += "file://0003-Add-OEM-Drives-parser.patch "
SRC_URI += "file://0004-Change-logging-to-stderr.patch "
SRC_URI += "file://0005-Add-debug-cmdline-param.patch "
SRC_URI += "file://0007-Add-Asus-OEM-NIC-record.patch "
SRC_URI += "file://0009-Don-t-allow-badly-formed-strings-in-SMBIOS-break-the.patch "
SRC_URI += "file://0010-Introduce-BIOS-release-date.patch "
SRC_URI += "file://0011-Implement-TPM-parsing.patch "
SRC_URI += "file://0012-Change-service-file.patch"
SRC_URI += "file://0013-Fixup-BIOS-version-and-release-date.patch"

DEPENDS:append = " cli11"
