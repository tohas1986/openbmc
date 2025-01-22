# Copyright (C) 2022 Konstantin Klubnichkin <kitsok@yandex-team.ru>
# Released under the MIT license (see COPYING.MIT for the terms)

DESCRIPTION = "Yandex specific configuration files"

inherit skeleton-rev

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=fa818a259cbed7ce8bc2a22d35a464fc"

FILESEXTRAPATHS:append := "${THISDIR}/files:"

RDEPENDS:${PN} = "ntp rsyslog"

SRC_URI = " \
    file://LICENSE \
    file://trapdoor.tcp \
    file://trapdoor.udp \
    file://ntpdate.conf \
    file://ntp.conf \
    "

S = "${WORKDIR}"

FILES:${PN} = " /etc/rsyslog.d/trapdoor.udp /etc/rsyslog.d/trapdoor.tcp /etc/rsyslog.d/trapdoor.conf /etc/default/ntpdate /etc/ntp.conf"

DIRFILES = "1"

do_install () {
    install -d -m 0644 ${D}/etc
    install -d -m 0644 ${D}/etc/default
    install -d -m 0644 ${D}/etc/rsyslog.d
    
    install -m 0644 ${WORKDIR}/ntp.conf ${D}/etc/ntp.conf
    install -m 0644 ${WORKDIR}/ntpdate.conf ${D}/etc/default/ntpdate
    install -m 0644 ${WORKDIR}/trapdoor.* ${D}/etc/rsyslog.d/
    install -m 0644 ${WORKDIR}/trapdoor.tcp ${D}/etc/rsyslog.d/trapdoor.conf
}
