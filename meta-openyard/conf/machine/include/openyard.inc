MACHINE_FEATURES += "\
        obmc-phosphor-fan-mgmt \
        obmc-host-state-mgmt \
        obmc-chassis-state-mgmt \
        obmc-bmc-state-mgmt \
        obmc-host-ipmi \
        obmc-phosphor-chassis-mgmt \ 
        obmc-phosphor-flash-mgmt \
        "

#VIRTUAL-RUNTIME_skeleton_workbook = "${MACHINE}-config"

PREFERRED_PROVIDER_virtual/obmc-chassis-mgmt = "packagegroup-openyard-apps"
PREFERRED_PROVIDER_virtual/obmc-fan-mgmt = "packagegroup-openyard-apps"
PREFERRED_PROVIDER_virtual/obmc-flash-mgmt = "packagegroup-openyard-apps"
PREFERRED_PROVIDER_virtual/obmc-system-mgmt = "packagegroup-openyard-apps"
#PREFERRED_PROVIDER_virtual/obmc-inventory-data ?= "${VIRTUAL-RUNTIME_skeleton_workbook}"
PREFERRED_PROVIDER_virtual/obmc-host-ipmi-hw = "phosphor-ipmi-kcs"

VIRTUAL-RUNTIME_obmc-host-state-manager ?= "x86-power-control"
VIRTUAL-RUNTIME_obmc-chassis-state-manager ?= "x86-power-control"
         

OBMC_ORG_YAML_SUBDIRS += "com/intel"

OVERRIDES .= ":openyard"
