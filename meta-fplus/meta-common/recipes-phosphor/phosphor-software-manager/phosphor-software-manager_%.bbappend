PV = "1.0+git${SRCPV}"

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-Append-a-random-number-after-the-original-version-ha.patch"
SRC_URI += "file://0002-Copy-image-files-together-with-signature-files.patch"
SRC_URI += "file://0004-Add-FPGA-update-support.patch"
SRC_URI += "file://0005-Don-t-call-resetUbootEnvVars-as-it-breaks-phosphor-i.patch"
SRC_URI += "file://0006-Extend-progress-bar-for-BIOS-update.patch"
SRC_URI += "file://0007-Disable-signature-verification-for-host-FW-update.patch"

SRC_URI += "file://bios-update.sh"
SRC_URI += "file://fpga-update.sh"

PACKAGECONFIG[flash_fpga] = "-Djbog-fpga-upgrade=enabled, -Djbog-fpga-upgrade=disabled"

PACKAGECONFIG:append = " flash_bios verify_signature flash_fpga"
PACKAGECONFIG:remove = "usb_code_update"
RDEPENDS:${PN} += "bash"

FILES:${PN}-updater += "${sbindir}/bios-update.sh"

FILES:${PN}-updater += "${sbindir}/fpga-update.sh"

SYSTEMD_SERVICE:${PN}-updater += "${@bb.utils.contains('PACKAGECONFIG', 'flash_fpga', 'obmc-flash-nvidia-jbog-fpga@.service', '', d)}"

do_install:append () {
    install -d ${D}/usr/sbin/
    install -m 0755 ${WORKDIR}/bios-update.sh ${D}/usr/sbin/
    install -m 0755 ${WORKDIR}/fpga-update.sh ${D}/usr/sbin/
}

FILES:${PN} += "${systemd_system_unitdir}/*.service"
