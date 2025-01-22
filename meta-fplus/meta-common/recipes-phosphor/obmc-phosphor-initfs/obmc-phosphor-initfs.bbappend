S = "${WORKDIR}"

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://obmc-update-yandex.sh"
SRC_URI += "file://obmc-shutdown-yandex.sh"
SRC_URI += "file://obmc-init-yandex.sh"
SRC_URI += "file://obmc-copyflash-yandex.sh"

do_install:append() {
	#TODO: refactor this to OpenYard features
	install -m 0755 ${WORKDIR}/obmc-update-yandex.sh ${D}/update
	install -m 0755 ${WORKDIR}/obmc-shutdown-yandex.sh ${D}/shutdown
	install -m 0755 ${WORKDIR}/obmc-init-yandex.sh ${D}/init
	install -m 0755 ${WORKDIR}/obmc-copyflash-yandex.sh ${D}/copyflash

	#bbplain "DEBUG: Fetching public key from YAV"
	#export SSH_AUTH_SOCK=${SSH_AUTH_SOCK}
	#/usr/bin/yav get version sec-01csy97rg46f9d5z2cpzm0nrge -O pub > ${WORKDIR}/publickey

	#install -d ${D}/etc
	#install -m 644 ${WORKDIR}/publickey ${D}/etc
}

FILES:${PN} += "/copyflash"

RDEPENDS:${PN} += "libgpiod-tools"
