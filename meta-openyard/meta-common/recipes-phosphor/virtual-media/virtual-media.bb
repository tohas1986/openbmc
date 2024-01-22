SUMMARY = "Virtual Media Service"
DESCRIPTION = "Virtual Media Service"

#SRC_URI = "git://git@github.com/Intel-BMC/virtual-media.git;protocol=ssh;branch=main"
#SRCREV = "78fd799195824eb444a539bacc05ec4587205fd7"

SRC_URI = "git://10.227.77.30:8929/kapitanov/virtual-media.git;protocol=http;branch=master"
SRCREV = "89a1121093b0c598d414511755b9f37227842fee"

S = "${WORKDIR}/git"
PV = "0.1+git${SRCPV}"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${S}/LICENSE;md5=e3fc50a88d0a364313df4b21ef20c29e"

SYSTEMD_SERVICE:${PN} += "xyz.openbmc_project.VirtualMedia.service"

DEPENDS = "udev boost nlohmann-json systemd sdbusplus"

RDEPENDS:${PN} = "nbd-client nbdkit"

inherit cmake systemd

EXTRA_OECMAKE += "-DYOCTO_DEPENDENCIES=ON"
EXTRA_OECMAKE += "-DLEGACY_MODE_ENABLED=ON"

#FULL_OPTIMIZATION = "-Os -pipe -flto -fno-rtti"
