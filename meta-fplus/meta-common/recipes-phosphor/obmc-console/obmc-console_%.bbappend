FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-Increase-ring-buffer-size.patch"
SRC_URI += "file://0004-Set-StartLimiBurst-to-really-big-number.patch"
SRC_URI += "file://0006-Increase-logfile-to-32Mb.patch"
SRC_URI += "file://0007-Disable-warning.patch"
SRC_URI += "file://0008-Enable-hardware-flow-control.patch"
SRC_URI += "file://0009-Add-sirq_polarity-option.patch"

SRC_URI += "file://server.ttyS0.conf.ai1"
SRC_URI += "file://server.ttyS3.conf.default"

SRC_URI:remove = "file://${BPN}.conf"

do_install:append() {
        # Remove upstream-provided configuration
        rm -f ${D}${sysconfdir}/${BPN}/server.*.conf

        # Install server configuration
        install -m 0755 -d ${D}${sysconfdir}/${BPN}
        local file=$(basename ${WORKDIR}/server.*.conf.${MACHINE} .${MACHINE})
        if [ -f "${WORKDIR}/${file}.${MACHINE}" ]; then
            install -m 0644 ${WORKDIR}/$file.${MACHINE} ${D}${sysconfdir}/${BPN}/${file}
        else
            # Default
            install -m 0644 ${WORKDIR}/server.ttyS3.conf.default ${D}${sysconfdir}/${BPN}/server.ttyS3.conf
        fi
}
