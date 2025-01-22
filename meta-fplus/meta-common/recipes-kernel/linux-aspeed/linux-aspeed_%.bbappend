FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"

SRC_URI:append:ai1 = " file://yandex-g5.cfg "
SRC_URI:append:my81 = " file://yandex-g5.cfg "
SRC_URI:append:mz81 = " file://yandex-g5.cfg "
SRC_URI:append:my62 = " file://yandex-g5.cfg "
SRC_URI:append:mzb2 = " file://yandex-g5.cfg "
SRC_URI:append:gd1 = " file://yandex-g5.cfg "
SRC_URI:append:gd2 = " file://yandex-g5.cfg "
SRC_URI:append:g1 = " file://yandex-g5.cfg "
SRC_URI:append:fp01 = " file://yandex-g5.cfg "

# This change removes the register check condition from the pass-
# through and desired SIO GPIOs so they can be requsted and
# monitored from power control.
SRC_URI += "file://0006-Allow-monitoring-of-power-control-input-GPIOs.patch"

# Add ESPI description to Aspeed-g5 dtsi
SRC_URI += "file://0014-arm-dts-aspeed-g5-add-espi.patch"

# Add SGPIO description to G4 and G5 DTS. Fixup pinctrl
SRC_URI += "file://0017-SGPIO-DT-and-pinctrl-fixup.patch"

# Add I2C slave mqueue support
SRC_URI += "file://0019-Add-I2C-IPMB-support.patch"

# Add debug to I2C drivers. Nice to have
SRC_URI += "file://0030-Add-dump-debug-code-into-I2C-drivers.patch"

# Adds high speed to UART
SRC_URI += "file://0031-Add-high-speed-baud-rate-support-for-UART.patch"

# Add UART routing (devices to HW pins) via sysfs
SRC_URI += "file://0032-misc-aspeed-Add-Aspeed-UART-routing-control-driver.patch"

# Implement VGA shared memory driver - UPSTREAMED
# Update 25.05.2020 Not upstreamed or removed
SRC_URI += "file://0035-Implement-a-memory-driver-share-memory.patch"

# I2C MUX hold/unhold messages
SRC_URI += "file://0040-i2c-Add-mux-hold-unhold-msg-types.patch"

# I2C timeout and retry DTS props for multi-master environment
SRC_URI += "file://0042-Add-bus-timeout-ms-and-retries-device-tree-propertie.patch"

# Some LPC BT patch
SRC_URI += "file://0043-char-ipmi-Add-clock-control-logic-into-Aspeed-LPC-BT.patch"

# Add R/W counters for emulated USB mass storage devices
SRC_URI += "file://0073-Add-IO-statistics-to-USB-Mass-storage-gadget.patch"

SRC_URI += "file://0091-Remove-COND2-from-TXD1-RXD1-pinctrl.patch"

# INA219 patch from RMM
SRC_URI += "file://0100-Add-calculated-power-with-linear-correction.patch"

SRC_URI += "file://0103-reduce-i2c-warning.patch"
SRC_URI += "file://0104-ina2xx-instantiate-device-even-if-error-occured.patch"
SRC_URI += "file://0105-Instantiate-pca954x-mux-anyway.patch" 
SRC_URI += "file://0108-adm1275-set-default-shunt-to-0.5mOhm.patch"
SRC_URI += "file://0110-jc42-Hack-the-driver-to-instantiate-devices-despite-.patch"
SRC_URI += "file://0111-Add-63M-flash-layout.patch"
# TO BE REMOVED
# SRC_URI += "file://0112-SMC-Don-t-unregister-whole-chip-if-one-flash-IC-is-m.patch"
# TO BE REMOVED
# SRC_URI += "file://0113-Fix-typo-in-flash-layout-dtsi.patch"
SRC_URI += "file://0115-Hack-Set-mem.devmem-1-by-default.patch"
SRC_URI += "file://0117-ina219-Cosmetics-Set-128-ADC-samples-overage-by-defa.patch"
SRC_URI += "file://0118-ina219-fix-unsigned-signed.patch"
SRC_URI += "file://0119-aspeed-lpc-snoop-introduce-de-duplication-in-IRQ-han.patch"
SRC_URI += "file://0120-pwmtacho-return-0-instead-of-error.patch"
SRC_URI += "file://0121-aspeed_kcs-Turn-on-clock-before-device-to-avoid-kern.patch"
# HACK HACK HACK
SRC_URI += "file://0122-lpc_snoop-Don-t-enable-interrupts-it-s-done-in-bmc-r.patch"
SRC_URI += "file://0125-nct7601-driver.patch"
SRC_URI += "file://0126-TMP421-read-control-register.patch"
SRC_URI += "file://0127-Add-lm5066i05-lm5066i2-device-for-LM5066i-with-0.5mO.patch"
SRC_URI += "file://0130-nct7802-Add-postponed-chip-init.patch"
SRC_URI += "file://0132-Add-generic-PMBUS-DCDC54-converter.patch"
SRC_URI += "file://0134-compiler.h-only-include-asm-rwonce.h-for-kernel-code.patch"
SRC_URI += "file://0135-i2c-aspeed-Don-t-attempt-to-do-xfer-on-locked-bus.patch"
SRC_URI += "file://0136-Fix-aspeed-vhub-oops.patch"
SRC_URI += "file://0137-usb-gadget-Remove-maximum-image-size-check.patch"
SRC_URI += "file://0138-adt7462-Add-manual-fan-control.patch"
SRC_URI += "file://0139-adt7462-add-max-fan-speed-control.patch"
SRC_URI += "file://0140-DT-Overlay-configfs-interface-from-RPi.patch"
SRC_URI += "file://0141-DVD-support-to-usb_gadget-https-patchwork.kernel.org.patch"
SRC_URI += "file://0142-Mark-UART1-2-clocks-as-critical.patch"
# Add AST2500 misc drivers from Intel repo https://github.com/Intel-BMC/linux/tree/dev-5.15-intel/drivers/soc/aspeed
SRC_URI += "file://0145-Add-LPC-eSPI-bmc-misc-drivers-from-Intel-repo.patch"
SRC_URI += "file://0148-AspeedTech-drivers.patch"
SRC_URI += "file://0149-ast2500-adc-Don-t-use-prescaler-due-to-ADC-silicon-i.patch"
SRC_URI += "file://0151-Increase-default-vmalloc-size-to-1Gb.patch"
SRC_URI += "file://0154-lm75-Introduce-postponed-initialization.patch"
SRC_URI += "file://0156-jtag-Switch-to-Ampere-branch-driver.patch"

# Uncomment to enable AST2600 HACE/ACRY
# SRC_URI += "file://0157-Add-Aspeed-crypto-drivers.patch"
# SRC_URI += "file://0158-WIP-Cryptodev.patch"

SRC_URI += "file://0159-aspeed-video-Reduce-spam-on-switched-on-host.patch"
SRC_URI += "file://0160-Fix-configfs-gadget-is-already-registered.patch"

SRC_URI += "file://dts/"

do_patch:append() {
	install -m 0644 ${WORKDIR}/dts/aspeed-yandex-*.dts ${STAGING_KERNEL_DIR}/arch/arm/boot/dts/
	git -C ${STAGING_KERNEL_DIR} add arch/arm/boot/dts/aspeed-yandex-*.dts
	git -C ${STAGING_KERNEL_DIR} commit -a -m "Add Yandex DTS files"
}
