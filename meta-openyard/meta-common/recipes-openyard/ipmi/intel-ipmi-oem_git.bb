SUMMARY = "Intel OEM IPMI commands"
DESCRIPTION = "Intel OEM IPMI commands"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=a6a4edad4aed50f39a66d098d74b265b"

#SRC_URI = "git://github.com/openbmc/intel-ipmi-oem;branch=master;protocol=https"
#SRCREV = "80d4d5f95e3a5845cd4d27a74ecb01a292d7e40d"

SRC_URI = "git://10.227.77.30:8929/kapitanov/intel-ipmi-oem.git;branch=master;protocol=http"
SRCREV = "2125cbfc48e7fdfd09d9515ea2b24c81e374f548"

S = "${WORKDIR}/git"
PV = "0.1+git${SRCPV}"

DEPENDS = "boost phosphor-ipmi-host phosphor-logging systemd phosphor-dbus-interfaces libgpiod"

inherit cmake obmc-phosphor-ipmiprovider-symlink pkgconfig

EXTRA_OECMAKE="-DENABLE_TEST=0 -DYOCTO=1"

LIBRARY_NAMES = "libzinteloemcmds.so"

HOSTIPMI_PROVIDER_LIBRARY += "${LIBRARY_NAMES}"
NETIPMI_PROVIDER_LIBRARY += "${LIBRARY_NAMES}"

FILES:${PN}:append = " ${libdir}/ipmid-providers/lib*${SOLIBS}"
FILES:${PN}:append = " ${libdir}/host-ipmid/lib*${SOLIBS}"
FILES:${PN}:append = " ${libdir}/net-ipmid/lib*${SOLIBS}"
FILES:${PN}-dev:append = " ${libdir}/ipmid-providers/lib*${SOLIBSDEV}"

do_install:append(){
   install -d ${D}${includedir}/intel-ipmi-oem
   install -m 0644 -D ${S}/include/*.hpp ${D}${includedir}/intel-ipmi-oem
}
