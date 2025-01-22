SUMMARY = "Default Fru"
DESCRIPTION = "Builds a default FRU file at runtime based on board ID"

inherit systemd
inherit cmake pkgconfig

#SYSTEMD_SERVICE:${PN} = "SetBaseboardFru.service"

S = "${WORKDIR}"
SRC_URI = "file://mkfru.cpp \
           file://CMakeLists.txt \
           file://rapidxml.hpp \
           file://rapidxml_iterators.hpp \
           file://rapidxml_print.hpp \
           file://rapidxml_utils.hpp \
           "

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "\
    file://${YANDEXBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658 \
    file://mkfru.cpp;beginline=2;endline=14;md5=c451359f18a13ee69602afce1588c01a \
    "

RDEPENDS:${PN} = "bash"
