do_install:append:openyard(){
  install -d ${D}${includedir}/phosphor-ipmi-host
  install -m 0644 -D ${S}/sensorhandler.hpp ${D}${includedir}/phosphor-ipmi-host
  install -m 0644 -D ${S}/selutility.hpp ${D}${includedir}/phosphor-ipmi-host
}

DEPENDS:append:openyard = " openyard-yaml-config"

EXTRA_OEMESON:openyard = " \
    -Dfru-yaml-gen=${STAGING_DIR_HOST}${datadir}/openyard-yaml-config/ipmi-fru-read.yaml \
    "

