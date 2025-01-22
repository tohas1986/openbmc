SUMMARY = "OpenBMC for Intel - Applications"


inherit packagegroup

PROVIDES = "${PACKAGES}"

#PACKAGES = " \
#        ${PN}-chassis \
#        ${PN}-fans \
#        ${PN}-flash \
#        ${PN}-system \
#        "

PACKAGES = "${PN}-system"

PROVIDES += "virtual/obmc-system-mgmt"
RPROVIDES:${PN}-system += "virtual-obmc-system-mgmt"

SUMMARY:${PN}-system = "Yandex System"
RDEPENDS:${PN}-system = " \
        bmcweb \
        entity-manager \
        intel-ipmi-oem \
        yandex-ipmi-oem \
        dbus-sensors \
        phosphor-host-postd \
        phosphor-inventory-manager \
        "

# Taken from FB, failed
# phosphor-post-code-manager
# phosphor-nvme - not yet
