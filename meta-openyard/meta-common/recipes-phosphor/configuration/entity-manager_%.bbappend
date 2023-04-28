FILESEXTRAPATHS:append := ":${THISDIR}/${PN}"
SRC_URI:append = " file://openyard-baseboard.json "

do_install:append() {
     rm -f ${D}${datadir}/entity-manager/configurations/*.json
     install -d ${D}${datadir}/entity-manager/configurations
     install -m 0444 ${WORKDIR}/openyard-baseboard.json ${D}${datadir}/entity-manager/configurations
}
