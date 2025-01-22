OBMC_MACHINE_FEATURES += "\
        obmc-host-ipmi \
        obmc-host-state-mgmt \
        obmc-chassis-state-mgmt \
        obmc-bmc-state-mgmt \
        "

PREFERRED_PROVIDER_virtual/obmc-host-ipmi-hw = "phosphor-ipmi-kcs"
#PREFERRED_PROVIDER_virtual/phosphor-led-manager-config-native = "my81-led-manager-config-native"
#PREFERRED_PROVIDER_u-boot-fw-utils ?= "u-boot-fw-utils-aspeed"
PREFERRED_PROVIDER_virtual/obmc-inventory-data = "entity-manager"
PREFERRED_PROVIDER_virtual/obmc-system-mgmt = "packagegroup-yandex-mgmt"
PREFERRED_PROVIDER_virtual/phosphor-led-manager-config-native = "common-led-manager-config-native"

VIRTUAL-RUNTIME_obmc-chassis-state-manager = "x86-power-control-yandex"
VIRTUAL-RUNTIME_obmc-host-state-manager = "x86-power-control-yandex"
VIRTUAL-RUNTIME_obmc-inventory-manager = "entity-manager"

# u-boot-fw-utils-aspeed "

DISTRO_FEATURES:remove = "avahi"
VIRTUAL-RUNTIME_obmc-sensors-hwmon = " "
VIRTUAL-RUNTIME_obmc-dump-monitor = " "

FEATURE_PACKAGES_obmc-health-monitor = ""

IMAGE_INSTALL += " ipmitool bmcweb screen sh-functions lmsensors-sensors "
IMAGE_INSTALL += " intel-ipmi-oem board-setup strace tcpdump lpc-cmds"
IMAGE_INSTALL += " netbase "
IMAGE_INSTALL += " usbnet-start lldp-forwarder "
IMAGE_INSTALL += " phosphor-ipmi-flash phosphor-logging phosphor-dbus-interfaces phosphor-ipmi-config phosphor-ipmi-net phosphor-ipmi-kcs default-fru "
IMAGE_INSTALL += " yandex-ipmi-oem phosphor-sel-logger phosphor-ipmi-ipmb curl dmidecode phosphor-host-postd admin-user phosphor-watchdog "
IMAGE_INSTALL += " phosphor-snmp virtual-media ntp ntp-utils ntpdate dbus-checker "
IMAGE_INSTALL += " phosphor-post-code-manager x86-power-control-yandex smbios-mdr webui-vue jq ethtool srvcfg-manager "
IMAGE_INSTALL += " phosphor-led-manager phosphor-pid-control "
IMAGE_INSTALL += " phosphor-ipmi-host dbus-sensors libpeci iotools openocd"
IMAGE_INSTALL += " peci-pcie"
IMAGE_INSTALL += " dimm-serial"

# FIXME Must be removed after IPv4 address configured
# IMAGE_INSTALL += " ampere-openocd "
IMAGE_INSTALL += " inventory-job mtd-utils-jffs2 config-saver id-button "
IMAGE_INSTALL += " iptables "
#IMAGE_INSTALL += " dobby "
IMAGE_INSTALL += "phosphor-state-manager"
IMAGE_INSTALL += "host-reboot"

# Uncomment to enable AST2600 HACE/ACRY
# IMAGE_INSTALL += " openssl-engines "

# DL_DIR = "/work/obmc_downloads"
# SSTATE_DIR = "/work/sstate-dir"

# See https://st.yandex-team.ru/RNDEMBED-639
#DL_DIR ?= "${TOPDIR}/../downloads"
#BB_GENERATE_MIRROR_TARBALLS ?= "1"
#SOURCE_MIRROR_URL ?= "https://s3.mds.yandex.net/hwrnd-openbmc-cache/downloads/"
#INHERIT += "own-mirrors"

# See https://st.yandex-team.ru/RNDEMBED-639
#SSTATE_DIR ?= "${TOPDIR}/../sstate-cache"
#SSTATE_MIRRORS ?= "file://.* https://s3.mds.yandex.net/hwrnd-openbmc-cache/sstate-cache/PATH;downloadfilename=PATH"

# BB_NO_NETWORK ?= "1"
# BB_FETCH_PREMIRRORONLY ?= "1"
# SSTATE_MIRROR_ALLOW_NETWORK ?= "1"

# export YAV_TOKEN to allow /usr/bin/yav run in environments without proper SSH key (CI)
#export YAV_TOKEN
OBMC_ORG_YAML_SUBDIRS += "com/intel"



