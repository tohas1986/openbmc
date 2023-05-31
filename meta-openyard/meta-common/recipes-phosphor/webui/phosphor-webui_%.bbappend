FILESEXTRAPATHS:prepend:openyard := "${THISDIR}/${PN}:"

SRC_URI:append:openyard = " file://config.json"

do_compile:prepend:intel() {
        cp -r ${WORKDIR}/config.json ${S}/
}

