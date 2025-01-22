EXTRA_OECMAKE += "${@bb.utils.contains('IMAGE_FSTYPES', 'intel-pfr', '-DINTEL_PFR_ENABLED=ON', '', d)}"

SRC_URI = "git://github.com/openbmc/intel-ipmi-oem;protocol=https;branch=master \
           file://0001-Replace-space-by-underscore-in-sensor-name.patch \
           file://0002-Disable-whitelist.patch \
           file://0003-Get-rid-of-Failed-to-GetAll-error-message.patch \
           file://0004-Change-ipmi_sel-location-to-persistent-folder.patch \
           file://0006-Remove-powerDownAcFailed.patch \
           file://0007-Disable-Set-SEL-Time.patch \
           file://0008-Add-write-to-IPMI-SEL-in-ipmiStorageAddSELEntry.patch \
           file://0009-Remove-Intel-OEM-NodeManager-SDR-record-from-output-.patch \
           file://0010-Fix-percent-in-ipmitool-output.patch \
           file://0011-Set-all-time-manufacturing-mode.patch \
           file://0012-Fix-ipmitool-bug-with-multi-record-area-offset-calcu.patch \
           file://0013-Clear-redfish-logs-by-sel-clear-cmd.patch \
           file://0014-Fix-min-0-and-max-0.patch \
           file://0015-Wrap-getACFailedStatus-to-return-always-false-as-we-.patch \
           file://0016-Fix-sel-logging.patch \
           file://0017-Remove-non-informative-harmless-error-message.patch \
           file://0018-Add-ME-message-sel-logging.patch \
           file://0019-Restart-service-on-sensors-restart.patch \
           file://0020-Reduce-caching-time-for-sensor-reading-function.patch \
           file://0021-Set-SDR-timestamp-to-now-at-startup.patch \
           file://0024-Update-version-string-parser.patch \
           file://0025-Reduce-spamming.patch \
           file://0026-Fix-bus-number-type-after-entity-manager-update.patch \
           file://0027-Fix-stupid-untested-Google-assumption-change.patch \
           file://0028-Add-more-verbosity-to-error-messages.patch \
           "
