SUMMARY = "NVIDIA smbus post box sevice"
DESCRIPTION = "In case you need to get data from NVIDIA Delta board or Delta NEXT board"

#SRC_URI = "git://git@git.arc-vcs.yandex-team.ru/hwrnd-nvidia-smbus-post-box;protocol=https;branch=trunk"
#SRCREV = "928b67f363c4dda53203c002f21736e71b336be0"

SRC_URI = "git://github.com/tohas1986/nvidia-smbus-post-box.git;protocol=https;branch=main"
SRCREV = "3d05d76a5eecd7355b4d719808ab0bdbbf334625"


LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=e3fc50a88d0a364313df4b21ef20c29e"

SYSTEMD_SERVICE:${PN} = "nvidia-smbus-post-box.service nvidia-smbus-post-box.timer"

DEPENDS = "sdbusplus \
           phosphor-logging \
           boost \
		   nlohmann-json \
           i2c-tools \
           cli11 "

S = "${WORKDIR}/git"
inherit cmake pkgconfig systemd

pkg_postinst:${PN} () {
    if [ -n "$D" ]; then
        OPTS="--root=$D"
    fi

    if type systemctl >/dev/null 2>/dev/null; then
        systemctl $OPTS enable nvidia-smbus-post-box.timer
    fi
}

FILES:${PN} += "${sbindir}/nvidia-smbus-post-box ${systemd_unitdir}/system/nvidia-smbus-post-box.*"
