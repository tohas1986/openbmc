FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"
SRC_URI:append = " \
  file://dalegibbs.cfg \
  file://dts/ \
  "


do_patch:append() {
	install -m 0644 ${WORKDIR}/dts/aspeed-ucode-*.dts ${STAGING_KERNEL_DIR}/arch/arm/boot/dts/aspeed/
	git -C ${STAGING_KERNEL_DIR} add arch/arm/boot/dts/asped/aspeed-ucode-*.dts
	git -C ${STAGING_KERNEL_DIR} commit -a -m "Add Microcode DTS files"
}
