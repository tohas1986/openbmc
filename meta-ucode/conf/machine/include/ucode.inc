DISTROOVERRIDES .= ":ucode"

MACHINE_FEATURES += "\
        obmc-bmc-state-mgmt \
        obmc-phosphor-fan-mgmt \
        obmc-phosphor-flash-mgmt \
        "

VIRTUAL-RUNTIME_skeleton_workbook = "${MACHINE}-config"

PREFERRED_PROVIDER_virtual/obmc-chassis-mgmt = "packagegroup-ucode-apps"
PREFERRED_PROVIDER_virtual/obmc-fan-mgmt = "packagegroup-ucode-apps"
PREFERRED_PROVIDER_virtual/obmc-flash-mgmt = "packagegroup-ucode-apps"
PREFERRED_PROVIDER_virtual/obmc-inventory-data ?= "${VIRTUAL-RUNTIME_skeleton_workbook}"
PREFERRED_PROVIDER_virtual/obmc-system-mgmt = "packagegroup-ucode-apps"

# IMAGE_FEATURES:remove = "obmc-ikvm"

SERIAL_CONSOLES:ucode = "115200;ttyS4"
