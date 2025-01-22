FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

# add a user called bmcweb for the server to assume
# bmcweb is part of group shadow for non-root pam authentication
USERADD_PARAM:${PN} = "-r -s /usr/sbin/nologin -d /home/bmcweb -m -G shadow bmcweb"

GROUPADD_PARAM:${PN} = "web; redfish "

# Enable CPU Log and Raw PECI support
#EXTRA_OECMAKE += "-DBMCWEB_ENABLE_REDFISH_CPU_LOG=ON"
#EXTRA_OECMAKE += "-DBMCWEB_ENABLE_REDFISH_RAW_PECI=OFF"

# Enable Redfish BMC Journal support
#EXTRA_OECMAKE += "-DBMCWEB_ENABLE_REDFISH_BMC_JOURNAL=ON"

# Enable PFR support
#EXTRA_OECMAKE += "-DBMCWEB_ENABLE_REDFISH_PFR_FEATURE=ON"
#EXTRA_OECMAKE = "-DCMAKE_BUILD_TYPE=Debug"

# Enable VirtualMedia support
#EXTRA_OECMAKE += "-DBMCWEB_ENABLE_VM_NBDPROXY=ON"
#EXTRA_OECMAKE += "-DBMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES=OFF"
#EXTRA_OECMAKE += "-DBMCWEB_ENABLE_REDFISH=ON"
#EXTRA_OECMAKE += "-DBMCWEB_HTTP_REQ_BODY_LIMIT_MB=128"

SRC_URI += "file://0001-Customization-for-Yandex-bmcweb.patch"
SRC_URI += "file://0002-Add-RMT-redfish-support.patch"
SRC_URI += "file://0004-Enable-nbdproxy.patch"
SRC_URI += "file://0005-Introduce-ReleaseDate-in-Versions.patch"
SRC_URI += "file://0006-Add-timestamp-to-task-messages.patch"
SRC_URI += "file://0007-Increase-firmware-upgrade-timeout-from-5-to-10-minut.patch"
SRC_URI += "file://0008-Add-StartLimitBurst.patch"
SRC_URI += "file://0009-Implement-the-SEL-feature-for-redfish-log-service.patch"
SRC_URI += "file://0010-Implement-static-redfish-v1-OEM-directory.patch"
SRC_URI += "file://0011-Disable-pass-of-username-and-password-in-doMountVmLe.patch"
SRC_URI += "file://0013-Add-MotherBoard-Error-Field-to-redfish.patch"
SRC_URI += "file://0014-Add-BW-redfish-support.patch"
SRC_URI += "file://0015-Persistent-configuration.patch"
SRC_URI += "file://0016-Add-Self-test-service-for-redfish.patch"
SRC_URI += "file://0017-Create-symlink-prior-to-service-start.patch"
SRC_URI += "file://0018-WIP-Rework-sel-to-redfish-logging.patch"
SRC_URI += "file://0019-Fix-BIOS-update-progress-bar.patch"
# The code in upstream is heavily modified and this patch doesn't seem to
# be needed anymore.
#SRC_URI += "file://0020-Fix-same-DIMM-id-request-for-redfish-core-lib-memory.patch"
SRC_URI += "file://0021-Introduce-new-redfish-service-for-NVIDIA-smbus-post.patch"
SRC_URI += "file://0022-Fix-failure-due-to-root-ADMIN-roles-separation.patch"
SRC_URI += "file://0023-Fix-method-check.patch"
SRC_URI += "file://0024-Fix-memory-mapping-on-Rome-Milan-AMD-4.0.patch"
SRC_URI += "file://0025-OpenYard-first-patch.patch"

# To enable logging add -Dbmcweb-logging=enabled
EXTRA_OEMESON = " \
        -Dhttp-body-limit=128 \
        -Dredfish=enabled \
        -Dredfish-bmc-journal=enabled \
        -Dredfish-dbus-log=disabled \
        -Dredfish-cpu-log=enabled \
        -Drest=enabled \
        "
