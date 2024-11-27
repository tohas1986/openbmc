DEPENDS:append = " breath-yaml-config"

EXTRA_OEMESON = " \
    -Dsensor-yaml-gen=${STAGING_DIR_HOST}${datadir}/breath-yaml-config/ipmi-sensors.yaml \
    -Dinvsensor-yaml-gen=${STAGING_DIR_HOST}${datadir}/breath-yaml-config/ipmi-inventory-sensors.yaml \
    -Dfru-yaml-gen=${STAGING_DIR_HOST}${datadir}/breath-yaml-config/ipmi-fru-read.yaml \
    "
