DEPENDS:append = " breath-yaml-config"

EXTRA_OECONF = " \
    INVSENSOR_YAML_GEN=${STAGING_DIR_HOST}${datadir}/breath-yaml-config/ipmi-inventory-sensors.yaml \
    "
