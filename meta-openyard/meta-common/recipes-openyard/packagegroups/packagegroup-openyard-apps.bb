SUMMARY = "OpenBMC for Intel - Applications"
PR = "r1"

inherit packagegroup

PROVIDES = "${PACKAGES}"
PACKAGES = " \
        ${PN}-chassis \
        ${PN}-fans \
        ${PN}-flash \
        ${PN}-system \
        "

PROVIDES += "virtual/obmc-chassis-mgmt"
PROVIDES += "virtual/obmc-fan-mgmt"
PROVIDES += "virtual/obmc-flash-mgmt"
PROVIDES += "virtual/obmc-system-mgmt"

RPROVIDES:${PN}-chassis += "virtual-obmc-chassis-mgmt"
RPROVIDES:${PN}-fans += "virtual-obmc-fan-mgmt"
RPROVIDES:${PN}-flash += "virtual-obmc-flash-mgmt"
RPROVIDES:${PN}-system += "virtual-obmc-system-mgmt"

SUMMARY:${PN}-chassis = "Intel Chassis"
RDEPENDS:${PN}-chassis = " \
        obmc-host-failure-reboots \
        "

SUMMARY:${PN}-fans = "Intel Fans"
RDEPENDS:${PN}-fans = " \
        phosphor-pid-control \
        "

SUMMARY:${PN}-flash = "Intel Flash"
RDEPENDS:${PN}-flash = " \
        "

SUMMARY:${PN}-system = "Intel System"
RDEPENDS:${PN}-system = " \
        bmcweb \
        entity-manager \
        webui-vue \
        dbus-sensors \
        phosphor-virtual-sensor \
        "

RDEPENDS:${PN}-extras:remove = " phosphor-hwmon"
VIRTUAL-RUNTIME_obmc-sensors-hwmon ?= "dbus-sensors"
RDEPENDS:${PN}-extras:append = " phosphor-virtual-sensor"
