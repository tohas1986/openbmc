SUMMARY = "Default administrative account"
DESCRIPTION = "Creating default account for system administrator"


inherit useradd

# License info
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

# Dependencies
DEPENDS = "phosphor-ipmi-host \
           phosphor-user-manager \
           bmcweb "

# Default administrative account (login: admin, password: 0penBmc)
ADMIN_LOGIN = "ADMIN"
ADMIN_PASS = "\$1\$UGMqyqdG\$x92eaaFGjevvagbCUI5ZE1"
ADMIN_GROUP = "priv-admin"
ADMIN_GROUPS = "priv-admin,web,redfish,ipmi"
USERADD_PACKAGES = "${PN}"

USERADD_PARAM:${PN} = "--gid ${ADMIN_GROUP} \
                       --groups ${ADMIN_GROUPS} \
                       --password '${ADMIN_PASS}' \
                       --shell /bin/true \
                       --no-create-home \
                       --home-dir /tmp \
                       ${ADMIN_LOGIN}"

# We don't have package body
ALLOW_EMPTY:${PN} = "1"

# Workaround for meta-phosphor/classes/phosphor-rootfs-postcommands.bbclass.
# The bb-script cannot add root to non-empty groups (invalid sed command).
GROUPMEMS_PARAM:${PN} = "-a root -g adm;"
