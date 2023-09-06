FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = "\
	file://0001-Add-OY-PStype.patch	\
"

PACKAGECONFIG:dev2600 = " \
    adcsensor \
    intelcpusensor \
    fansensor \
    hwmontempsensor \
    ipmbsensor \
    psusensor \
"
