FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://ntpd.service"
SRC_URI += "file://ntpdate-sync.sh"

SYSTEMD_SERVICE:${PN} += "ntpd.service"

do_install:append() {
	rm -rf ${D}/${sysconfdir}/default/ntpdate || :
	rm -rf ${D}/${sysconfdir}/ntp.conf || :
	install -m 755 ${WORKDIR}/ntpdate-sync.sh ${D}${bindir}/ntpdate-sync
}
