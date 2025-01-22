SUMMARY = "Ampere Platform Environment Definitions"
DESCRIPTION = "Ampere Platform Environment Definitions"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

RDEPENDS:${PN} = "bash"
DEPENDS = "zlib"

SRC_URI = "git://github.com/ampere-openbmc/ampere-platform-mgmt.git;protocol=https;branch=ampere"

SRC_URI:append = " \
           file://ampere_add_redfishevent.sh \
           file://ampere_update_mac.sh \
          "
SRCREV = "26ec37cd8275e881c538677b4ebd0519e112b4bc"

S = "${WORKDIR}/git/utilities/flash"
ROOT = "${STAGING_DIR_TARGET}"

LDFLAGS += "-L ${ROOT}/usr/lib/ -lz "

do_compile() {
    ${CC} ${S}/ampere_eeprom_prog.c -o ${S}/ampere_eeprom_prog -I${ROOT}/usr/include/ ${LDFLAGS}
    ${CC} ${S}/nvparm.c -o ${S}/nvparm -I${ROOT}/usr/include/ ${LDFLAGS}
    ${CC} ${S}/ampere_fru_upgrade.c -o ${S}/ampere_fru_upgrade -I${ROOT}/usr/include/ ${LDFLAGS}
}

do_install() {
    install -d ${D}/usr/sbin
    install -m 0755 ${WORKDIR}/ampere_add_redfishevent.sh ${D}/${sbindir}/
    install -m 0755 ${WORKDIR}/ampere_update_mac.sh ${D}/${sbindir}/
    install -m 0755 ${S}/ampere_eeprom_prog ${D}/${sbindir}/ampere_eeprom_prog
    install -m 0755 ${S}/ampere_fru_upgrade ${D}/${sbindir}/ampere_fru_upgrade
}
