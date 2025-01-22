FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://blacklist-ai1.json"
SRC_URI += "file://0001-fru_device-logging.patch"
SRC_URI += "file://0002-Add-ADM1278-device.patch"
SRC_URI += "file://0003-fru_device-ignore-incorrect-CRC.patch"
SRC_URI += "file://0004-Introduce-GUID-parse-from-MultiPart-area.patch"
SRC_URI += "file://0005-Write-FRU-content-to-etc-system_fru.txt.patch"
SRC_URI += "file://0006-Fix-area-overlap-bug.patch"
SRC_URI += "file://0007-Fix-FRU-write-and-read.patch"
SRC_URI += "file://0008-Revert-devices-Remove-devices-now-managed-by-hwmonte.patch"

SRC_URI += "file://blacklist-${MACHINE}.json"
SRC_URI += "file://AI1.json"
SRC_URI += "file://G1.json"
SRC_URI += "file://GD1.json"
SRC_URI += "file://GD2.json"
SRC_URI += "file://GE2.json"
SRC_URI += "file://MY62.json"
SRC_URI += "file://MY81.json"
SRC_URI += "file://MZ81.json"
SRC_URI += "file://MZB2.json"
SRC_URI += "file://EVB2600.json"
SRC_URI += "file://MR92.json"
SRC_URI += "file://SOM.json"
SRC_URI += "file://blacklist.json"

#Waiting upstream sync
#SRC_URI += "file://Y4BR-Y1-GPU-RX2.json"

# In upper recipe
#SRC_URI += "file://blocklist.json"

# WIP SRCREV = "b3e81100b264d9459109f796b5cf1bb169331134"

EXTRA_OECMAKE = "-DYOCTO=1 -DUSE_OVERLAYS=0"
CXXFLAGS:append = " -Wno-psabi -Wno-sign-compare "

DEPENDS:append = " cli11"

do_install:append() {
    # Remove upstream-provided configuration
    rm -f ${D}/usr/share/entity-manager/configurations/*.json

    # Install servers configuration
    for i in ${WORKDIR}/*.json; do
        echo $i | grep -q blacklist >/dev/null && continue
        install -m 0444 $i ${D}/usr/share/entity-manager/configurations
    done

    # Install blacklist
    if [ -f "${WORKDIR}/blacklist-${MACHINE}.json" ]; then
        install -m 0644 ${WORKDIR}/blacklist-${MACHINE}.json ${D}/usr/share/entity-manager/blacklist.json
    else
    # Default
        install -m 0644 ${WORKDIR}/blacklist.json ${D}/usr/share/entity-manager/blacklist.json
    fi  
}
