SUMMARY = "Host memory application"

SRC_URI = "git://github.com/Intel-BMC/host-memory.git;protocol=https;branch=master"
SRCREV = "84f93930f87a673baa909dfecd2e47de82ae1e63"
PV = "0.1+git${SRCPV}"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${INTELBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

DEPENDS = "boost \
           sdbusplus \
           libpeci \
           libgpiod"

S = "${WORKDIR}/git"
inherit cmake pkgconfig systemd

SYSTEMD_SERVICE:${PN} = "xyz.openbmc_project.PMEM.service xyz.openbmc_project.CLTT.service"

PACKAGECONFIG ??= "${@bb.utils.filter('DISTRO_FEATURES', 'pmem-advanced-features', d)}"
PACKAGECONFIG[pmem-advanced-features] = "-DPMEM_ADVANCED_FEATURES=ON, -DPMEM_ADVANCED_FEATURES=OFF"
