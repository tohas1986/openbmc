DESCRIPTION = "Yandex certificate"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=fa818a259cbed7ce8bc2a22d35a464fc"

SRC_URI = "file://allCAs.pem"
SRC_URI += "file://LICENSE"

S = "${WORKDIR}"

DEPENDS = "ca-certificates"
RDEPENDS:${PN} = "ca-certificates"

do_install () {
    bbplain "DEBUG: installing Yandex allCAs.pem into ${D}"
    install -d ${D}${datadir}/ca-certificates/yandex/
    install -m 0755 ${S}/allCAs.pem ${D}${datadir}/ca-certificates/yandex/allCAs.crt
    install -d ${D}${sysconfdir}/ssl/certs/authority
    install -m 0755 ${S}/allCAs.pem ${D}${sysconfdir}/ssl/certs/authority/allCAs.crt
}

FILES:${PN} += "${datadir}/ca-certificates/yandex/allCAs.crt ${sysconfdir}/ssl/certs/authority/allCAs.crt"
