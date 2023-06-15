DEPENDS:append:openyard = " openyard-yaml-config"

EXTRA_OECONF:openyard = " \
    YAML_GEN=${STAGING_DIR_HOST}${datadir}/openyard-yaml-config/ipmi-fru-read.yaml \
    PROP_YAML=${STAGING_DIR_HOST}${datadir}/openyard-yaml-config/ipmi-extra-properties.yaml \
    "
