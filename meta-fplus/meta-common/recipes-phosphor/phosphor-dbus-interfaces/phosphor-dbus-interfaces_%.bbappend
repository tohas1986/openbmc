FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-Add-properties-to-Ethernet.interface.patch"
SRC_URI += "file://0002-Add-drive-interface.patch"
SRC_URI += "file://0004-Add-smbios-mdrv2-required-CPU-attributes.patch"
SRC_URI += "file://0005-Add-PreInterruptFlag-properity-in-DBUS.patch"
SRC_URI += "file://0006-Add-ReleaseDate-to-Revision-and-Software-Version-int.patch"
SRC_URI += "file://0007-Add-TPM-properties.patch"

#addtask do_update_yamls after do_configure before do_compile

do_update_yamls(){
	${WORKDIR}/recipe-sysroot-native/usr/bin/sdbus++-gen-meson --directory ${S} --command meson --output ${S}/gen
}
