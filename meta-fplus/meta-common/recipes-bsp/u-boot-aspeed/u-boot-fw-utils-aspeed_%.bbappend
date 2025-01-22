FILESEXTRAPATHS:append:= "${THISDIR}/${PN}:"

# the meta-phosphor layer adds this patch, which conflicts
# with the yandex/common layout for environment

SRC_URI += "file://fw_env.config"
