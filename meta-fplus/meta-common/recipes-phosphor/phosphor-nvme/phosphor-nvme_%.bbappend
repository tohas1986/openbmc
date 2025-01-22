FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://nvme_config.json"
SRC_URI += "file://0001-logging.patch"

FILES:${PN} += "${sysconfdir}/nvme/nvme_config.json"

do_install:append() {
    install -m 0644 -D ${WORKDIR}/nvme_config.json \
        ${D}${sysconfdir}/nvme/nvme_config.json
}
