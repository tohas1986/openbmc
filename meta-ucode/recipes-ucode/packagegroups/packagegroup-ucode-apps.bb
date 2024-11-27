SUMMARY = "OpenBMC for ucode - Applications"
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

SUMMARY:${PN}-chassis = "ucode OpenPOWER Chassis"
RDEPENDS:${PN}-chassis = " \
    obmc-phosphor-power \
    phosphor-skeleton-control-power \
"

# obmc-phosphor-buttons-signals
# obmc-phosphor-buttons-handler
# phosphor-power-control
# phosphor-post-code-manager
# phosphor-host-postd       
# phosphor-skeleton-control-power
# phosphor-ipmi-ipmb

# phosphor-power-control
# phosphor-power-utils
# phosphor-power

SUMMARY:${PN}-fans = "ucode Fans"
RDEPENDS:${PN}-fans = " \
    phosphor-pid-control \
    "

SUMMARY:${PN}-flash = "ucode Flash"
RDEPENDS:${PN}-flash = " \
        phosphor-software-manager \
        "


SUMMARY:${PN}-system = "ucode System"
RDEPENDS:${PN}-system = " \
    phosphor-sel-logger \
    phosphor-state-manager \
    smbios-mdr \
    "

# RDEPENDS:${PN}-inventory:append = " openpower-occ-control id-button"
