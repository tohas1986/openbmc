SUMMARY = "OpenBMC for Yandex - Applications"

inherit packagegroup

PACKAGES = "packagegroup-yandex-apps"

RDEPENDS:packagegroup-yandex-apps = " \
	hwwbus-hb \
	yandex-cert \
	template-service \
	rmt-parser \
	update-motd \
	ya-syncpass \
	bmc-self-test \
	yandex-configs \
	"

#pkg_postinst:${PN} () {
#    if [ -n "$D" ]; then
#        OPTS="--root=$D"
#    fi
#
#    if type systemctl >/dev/null 2>/dev/null; then
#        systemctl $OPTS enable dropbear.service
#    fi
#}

#	cauth-updater

