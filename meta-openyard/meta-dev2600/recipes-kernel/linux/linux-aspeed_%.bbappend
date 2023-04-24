FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"
SRC_URI += "file://aspeed-bmc-openyard-dev2600.dts \
            "

do_patch:append() {
  for DTB in "${KERNEL_DEVICETREE}"; do
      DT=`basename ${DTB} .dtb`
      if [ -r "${WORKDIR}/${DT}.dts" ]; then
          cp ${WORKDIR}/${DT}.dts \
              ${STAGING_KERNEL_DIR}/arch/${ARCH}/boot/dts
      fi
  done
 
}

