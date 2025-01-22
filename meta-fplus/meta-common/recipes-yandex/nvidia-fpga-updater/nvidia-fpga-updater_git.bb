SUMMARY = "Yandex OpenBmc sevice for nvidia FPGA update"
DESCRIPTION = "In case you need to upgrade NVDIA HGX A100 onboard FPGA"

SRC_URI = "git://10.227.72.24:8929/ya-openbmc/nvidia-fpga-updater.git;protocol=http;branch=main \
           file://0001-Fix-missing-include-Bump-CPP-standard-to-20.patch \
           "
SRCREV = "e9ab219ef5f354f671cb7faee82529cf44841286"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=e3fc50a88d0a364313df4b21ef20c29e"

DEPENDS = "sdbusplus \
           phosphor-logging \
           boost \
           i2c-tools \
           cli11"

S = "${WORKDIR}/git"
inherit cmake pkgconfig systemd
