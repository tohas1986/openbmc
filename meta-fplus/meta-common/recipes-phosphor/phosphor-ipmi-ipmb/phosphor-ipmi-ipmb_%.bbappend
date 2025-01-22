FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"
SRC_URI += "file://ipmb-channels_${MACHINE}.json"

SRC_URI += "file://0002-Revert-Remove-broadcast-limitation-of-ipmb-channel-p.patch"
SRC_URI += "file://0003-Add-debug-output.patch"

DEPENDS:append = " cli11"

do_install:append() {
	install -m 644 ${WORKDIR}/ipmb-channels_${MACHINE}.json ${D}${datadir}/ipmbbridge/ipmb-channels.json
}
