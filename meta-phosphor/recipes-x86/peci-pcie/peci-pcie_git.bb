SUMMARY = "PECI PCIe"
DESCRIPTION = "Gathers PCIe information using PECI \
and provides it on D-Bus"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=7becf906c8f8d03c237bad13bc3dac53"
DEPENDS = "boost sdbusplus libpeci"
#SRCREV = "985d9d9e2d7a870b4c160fc297df38ca8bdd1e3c"
SRCREV = "87d0cba5285f3d8e7dc0b438af3d2e1203f1f8a0"
PV = "0.1+git${SRCPV}"

#SRC_URI = "git://github.com/openbmc/peci-pcie;branch=master;protocol=https"
SRC_URI = "git://10.227.77.30:8929/kapitanov/peci-pcie.git;branch=master;protocol=http"

S = "${WORKDIR}/git"
SYSTEMD_SERVICE:${PN} += "xyz.openbmc_project.PCIe.service"

inherit cmake pkgconfig systemd

EXTRA_OECMAKE = "-DYOCTO=1"
