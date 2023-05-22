FILESEXTRAPATHS:prepend:openyard := "${THISDIR}/${PN}:"

inherit obmc-phosphor-systemd
SYSTEMD_SERVICE:${PN}:openyard = "phosphor-pid-control.service"
EXTRA_OECONF:openyard = "--enable-configure-dbus=yes"
