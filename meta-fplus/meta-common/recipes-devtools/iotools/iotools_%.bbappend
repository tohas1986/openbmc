
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

do_install:append() {
	ln -s iotools ${D}${sbindir}/and
	ln -s iotools ${D}${sbindir}/btr
	ln -s iotools ${D}${sbindir}/bts
	ln -s iotools ${D}${sbindir}/busy_loop
	ln -s iotools ${D}${sbindir}/cmos_read
	ln -s iotools ${D}${sbindir}/cmos_write
	ln -s iotools ${D}${sbindir}/cpu_list
	ln -s iotools ${D}${sbindir}/io_read16
	ln -s iotools ${D}${sbindir}/io_read32
	ln -s iotools ${D}${sbindir}/io_read8
	ln -s iotools ${D}${sbindir}/io_write16
	ln -s iotools ${D}${sbindir}/io_write32
	ln -s iotools ${D}${sbindir}/io_write8
	ln -s iotools ${D}${sbindir}/mem_dump
	ln -s iotools ${D}${sbindir}/mem_read16
	ln -s iotools ${D}${sbindir}/mem_read32
	ln -s iotools ${D}${sbindir}/mem_read64
	ln -s iotools ${D}${sbindir}/mem_read8
	ln -s iotools ${D}${sbindir}/mem_write16
	ln -s iotools ${D}${sbindir}/mem_write32
	ln -s iotools ${D}${sbindir}/mem_write64
	ln -s iotools ${D}${sbindir}/mem_write8
	ln -s iotools ${D}${sbindir}/mmio_dump
	ln -s iotools ${D}${sbindir}/mmio_read16
	ln -s iotools ${D}${sbindir}/mmio_read32
	ln -s iotools ${D}${sbindir}/mmio_read64
	ln -s iotools ${D}${sbindir}/mmio_read8
	ln -s iotools ${D}${sbindir}/mmio_write16
	ln -s iotools ${D}${sbindir}/mmio_write32
	ln -s iotools ${D}${sbindir}/mmio_write64
	ln -s iotools ${D}${sbindir}/mmio_write8
	ln -s iotools ${D}${sbindir}/not
	ln -s iotools ${D}${sbindir}/or
	ln -s iotools ${D}${sbindir}/pci_list
	ln -s iotools ${D}${sbindir}/pci_read16
	ln -s iotools ${D}${sbindir}/pci_read32
	ln -s iotools ${D}${sbindir}/pci_read8
	ln -s iotools ${D}${sbindir}/pci_write16
	ln -s iotools ${D}${sbindir}/pci_write32
	ln -s iotools ${D}${sbindir}/pci_write8
	ln -s iotools ${D}${sbindir}/runon
	ln -s iotools ${D}${sbindir}/shl
	ln -s iotools ${D}${sbindir}/shr
	ln -s iotools ${D}${sbindir}/smbus_block_process_call
	ln -s iotools ${D}${sbindir}/smbus_process_call
	ln -s iotools ${D}${sbindir}/smbus_quick
	ln -s iotools ${D}${sbindir}/smbus_read16
	ln -s iotools ${D}${sbindir}/smbus_read32
	ln -s iotools ${D}${sbindir}/smbus_read64
	ln -s iotools ${D}${sbindir}/smbus_read8
	ln -s iotools ${D}${sbindir}/smbus_readblock
	ln -s iotools ${D}${sbindir}/smbus_receive_byte
	ln -s iotools ${D}${sbindir}/smbus_send_byte
	ln -s iotools ${D}${sbindir}/smbus_write16
	ln -s iotools ${D}${sbindir}/smbus_write32
	ln -s iotools ${D}${sbindir}/smbus_write64
	ln -s iotools ${D}${sbindir}/smbus_write8
	ln -s iotools ${D}${sbindir}/smbus_writeblock
	ln -s iotools ${D}${sbindir}/xor
	rm -rf ${D}/etc/systemd/system/multi-user.target.wants/iotools-setup.service || :
	rm -rf ${D}/lib/systemd/system/iotools-setup.service || :
}

FILES:${PN}:remove = "/lib/systemd/system/iotools-setup.service"
SYSTEMD_SERVICE:${PN}:remove = "iotools-setup.service"
