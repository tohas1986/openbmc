FILESEXTRAPATHS:append := ":${THISDIR}/${PN}"

SRC_URI += "file://phosphor-health-monitor.service"

do_install:append() {
	install -m 0644 ${WORKDIR}/phosphor-health-monitor.service ${D}${systemd_system_unitdir}
}
