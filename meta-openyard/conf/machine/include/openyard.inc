MACHINE_FEATURES += "\
        obmc-bmc-state-mgmt \
        obmc-phosphor-fan-mgmt \
        obmc-phosphor-flash-mgmt \
        "

VIRTUAL-RUNTIME_skeleton_workbook = "${MACHINE}-config"

PREFERRED_PROVIDER_virtual/obmc-chassis-mgmt = "packagegroup-openyard-apps"
PREFERRED_PROVIDER_virtual/obmc-fan-mgmt = "packagegroup-openyard-apps"
PREFERRED_PROVIDER_virtual/obmc-flash-mgmt = "packagegroup-openyard-apps"
PREFERRED_PROVIDER_virtual/obmc-system-mgmt = "packagegroup-openyard-apps"
PREFERRED_PROVIDER_virtual/obmc-inventory-data ?= "${VIRTUAL-RUNTIME_skeleton_workbook}"
PREFERRED_PROVIDER_virtual/obmc-host-ipmi-hw = "phosphor-ipmi-kcs"

OVERRIDES .= ":openyard"
