FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0002-networkd-wait-online-Get-online-if-any-interface-is-.patch"
SRC_URI += "file://0003-Increase-restart-limit.patch"
SRC_URI += "file://0004-wait-online-fail-in-case-of-event-loop-error.patch"
SRC_URI += "file://0005-Disable-gpt-auto-generator.patch"
SRC_URI += "file://system.conf"

PACKAGECONFIG:remove = "timesyncd"
PACKAGECONFIG:remove = "blkid"

do_install:append() {
	# Remove upstream-provided configuration
	rm -f ${D}${sysconfdir}/systemd/system.conf

	# Install server configuration
	install -m 0755 -d ${D}${sysconfdir}/systemd
	install -m 0644 ${WORKDIR}/system.conf ${D}${sysconfdir}/systemd/system.conf
}
