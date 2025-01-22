FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

inherit yandex-uboot-sign

SRC_URI += "file://0001-Introduce-yandex_ast2600-board.patch"
SRC_URI += "file://0002-WIP-Yandex.patch"

# Per machine configuration
SRC_URI += " file://som2600v1_otp.json file://keys/ "
OTPTOOL_CONFIGS:som2600v1-sec = "${WORKDIR}/som2600v1_otp.json"
OTPTOOL_KEY_DIR:som2600v1-sec = "${WORKDIR}/keys/"
SOCSEC_SIGN_ENABLE:som2600v1-sec = "1"
SOCSEC_SIGN_KEY:som2600v1-sec = "${WORKDIR}/keys/rsa_oem_dss_key.pem"
