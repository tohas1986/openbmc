FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://02-disable-whitelist.patch"
SRC_URI += "file://03-Get-rid-of-strange-limitation-regarding-channel-and-.patch"
SRC_URI += "file://04-No-spam.patch"
SRC_URI += "file://06-Disable-Get-Set-ACPI.patch"
SRC_URI += "file://07-Add-get-set-SNMP-community.patch"
SRC_URI += "file://08-Adding-workaround-for-set-sel-time-cmd.patch"
SRC_URI += "file://13-Add-optional-LAN-get-set-params.patch"
SRC_URI += "file://14-Implement-chassis-bootparams-according-to-28.12-28.1.patch"
SRC_URI += "file://15-Disable-0x06-0x52-whitelisting.patch"
SRC_URI += "file://16-Don-t-restart-ntp.patch"
SRC_URI += "file://17-Modify-ipmitool-power-diag-command-to-work-with-x86-.patch"
SRC_URI += "file://18-Break-spec-use-7-bits-for-bus-number-in-master-write.patch"
SRC_URI += "file://19-Add-power-operation-guard-after-bmc-reset-cold.patch"
SRC_URI += "file://21-Implement-set-get-global-enables.patch"
SRC_URI += "file://22-Unknown-ipmiwhitelist.cpp-file.patch"
SRC_URI += "file://23-Reduce-spamming.patch"
SRC_URI += "file://24-Remove-NTP-check-from-storagehandler-SetSelTime.patch"

SRC_URI += "file://phosphor-ipmi-host.service"

#option to enable/disable safe mode in boot flags
#EXTRA_OECMAKE += "-DENABLE_BOOT_FLAG_SAFE_MODE_SUPPORT=OFF"

# remove the softpoweroff service since we do not need it
SYSTEMD_SERVICE:${PN}:remove = " \
    xyz.openbmc_project.Ipmi.Internal.SoftPowerOff.service"

SYSTEMD_LINK:${PN}:remove = " \
    ../xyz.openbmc_project.Ipmi.Internal.SoftPowerOff.service:obmc-host-shutdown@0.target.requires/xyz.openbmc_project.Ipmi.Internal.SoftPowerOff.service \
    "
FILES:${PN}:remove = " \
    ${systemd_unitdir}/system/obmc-host-shutdown@0.target.requires/ \
    ${systemd_unitdir}/system/obmc-host-shutdown@0.target.requires/xyz.openbmc_project.Ipmi.Internal.SoftPowerOff.service \
    "

do_install:append(){
  install -d ${D}${includedir}/phosphor-ipmi-host
  install -m 0644 -D ${S}/sensorhandler.hpp ${D}${includedir}/phosphor-ipmi-host
  install -m 0644 -D ${S}/selutility.hpp ${D}${includedir}/phosphor-ipmi-host
}
