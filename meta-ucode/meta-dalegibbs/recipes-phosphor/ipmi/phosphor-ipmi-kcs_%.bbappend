# Create additional kcsbridge service on /dev/ipmi-kcs1
KCS_DEVICE_ALT = "ipmi-kcs4"
SYSTEMD_SERVICE:${PN} += " ${PN}@${KCS_DEVICE_ALT}.service"
