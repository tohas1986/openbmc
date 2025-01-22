FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-No-veryfy-on-flash-write.patch"
SRC_URI += "file://udhcpc.cfg"
SRC_URI += "file://ps-extras.cfg"
SRC_URI += "file://netstat.cfg"
SRC_URI += "file://busybox-syslog.default"
SRC_URI += "file://syslogd.cfg"
SRC_URI += "file://lsof.cfg"
