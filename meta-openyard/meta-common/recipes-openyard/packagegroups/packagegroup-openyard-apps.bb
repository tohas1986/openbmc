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
        x86-power-control \
        "

SUMMARY:${PN}-fans = "Intel Fans"
RDEPENDS:${PN}-fans = " \
        "

SUMMARY:${PN}-flash = "Intel Flash"
RDEPENDS:${PN}-flash = " \
        phosphor-software-manager \ 
        "

SUMMARY:${PN}-system = "Intel System"
RDEPENDS:${PN}-system = " \
        bmcweb \
        entity-manager \
        webui-vue \
        dbus-sensors \
        phosphor-virtual-sensor \
        intel-ipmi-oem \
        ipmitool \
        oy-gpio \
        phosphor-ipmi-ipmb \
        smbios-mdr \
        iotools \
        phosphor-hwmon \
        peci-pcie \
        virtual-media \
        "

RDEPENDS:${PN}-extras:remove = " phosphor-host-state-manager"
RDEPENDS:${PN}-extras:remove = " phosphor-state-manager"
VIRTUAL-RUNTIME_obmc-sensors-hwmon ?= "dbus-sensors"
RDEPENDS:${PN}-extras:append = " phosphor-virtual-sensor"
