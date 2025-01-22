FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

#SYSTEMD_SUBSTITUTIONS:remove = "KCS_DEVICE:${KCS_DEVICE}:${DBUS_SERVICE_${PN}}"

# Default kcs device is ipmi-kcs3; this is SMS.
# Add SMM kcs device instance

# Replace the '-' to '_', since Dbus object/interface names do not allow '-'.
KCS_DEVICE = "ipmi_kcs3"
SMM_DEVICE = "ipmi_kcs4"
SYSTEMD_SERVICE:${PN}:append = " ${PN}@${SMM_DEVICE}.service "

SRC_URI += "file://99-ipmi-kcs.rules"
SRC_URI += "file://0003-Add-StartLimitBurst.patch"
SRC_URI += "file://0004-Introduce-communication-dump-with-v-option.patch"

do_install:append() {
    install -d ${D}${base_libdir}/udev/rules.d
    install -m 0644 ${WORKDIR}/99-ipmi-kcs.rules ${D}${base_libdir}/udev/rules.d/
    # install -d ${D}${systemd_unitdir}/system/
    # install -m 0644 ${WORKDIR}/phosphor-ipmi-kcs@.service ${D}${systemd_unitdir}/system/
}
