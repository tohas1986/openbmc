FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = "\
	file://0001-Add-OY-PStype.patch	\
"

PACKAGECONFIG = " \
    adcsensor \
    intelcpusensor \
    fansensor \
    hwmontempsensor \
    ipmbsensor \
    psusensor \
"
