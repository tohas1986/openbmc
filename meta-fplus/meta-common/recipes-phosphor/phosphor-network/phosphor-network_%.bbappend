FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0004-Add-parse-of-var-run-ignored_interfaces.txt.patch \
            file://0005-Reduce-logging.patch \
            file://0006-Add-parse-of-ntp.conf.patch \
            file://0007-Add-VendorClassIdentifier-to-interface-config-file.patch \
            file://0008-Set-usbnet-interfaces-unmanaged.patch \
            "

EXTRA_OECONF:append = " --enable-ipv6-accept-ra=yes"
EXTRA_OEMESON:append = " -Ddefault-ipv6-accept-ra=true"
EXTRA_OEMESON:append = " -Dvendor-class=\"InHouseBMC\""

# ################ BAD BREAKING COMMIT ##############
# SRCREV = "59e5b91d9784274d1d99b4c10e939c38606efacc"
#####################################################
SRCREV = "55bdc36cc22cf14ae4b1778264d3855d9d16bdd6"
