#!/bin/bash

echo Try to initialize BMC output GPIO

gpioset $(gpiofind BMC_INITDBP_CPU_PREQ_R_N)=1 >/dev/null 2>&1 && echo Setting BMC_INITDBP_CPU_PREQ_R_N to 1 || echo Unable to set BMC_INITDBP_CPU_PREQ_R_N
gpioset $(gpiofind FM_BMC_PCH_SCI_LPC_N)=1 >/dev/null 2>&1 && echo Setting FM_BMC_PCH_SCI_LPC_N to 1 || echo Unable to set FM_BMC_PCH_SCI_LPC_N
gpioset $(gpiofind IRQ_BMC_PCH_SMI_LPC_N)=1 >/dev/null 2>&1 && echo Setting IRQ_BMC_PCH_SMI_LPC_N to 1 || echo Unable to set IRQ_BMC_PCH_SMI_LPC_N
gpioset $(gpiofind FM_BMC_READY_N)=0 >/dev/null 2>&1 && echo Setting FM_BMC_READY_N to 1 || echo Unable to set FM_BMC_READY_N
gpioset $(gpiofind S3_MASK)=1 >/dev/null 2>&1 && echo Setting S3_MASK to 1 || echo Unable to set S3_MASK
gpioset $(gpiofind FM_BMC_CPU_PWR_DEBUG_R_N)=1 >/dev/null 2>&1 && echo Setting FM_BMC_CPU_PWR_DEBUG_R_N to 1 || echo Unable to set FM_BMC_CPU_PWR_DEBUG_R_N
gpioset $(gpiofind FM_BMC_DBP_PRESENT_R_N)=1 >/dev/null 2>&1 && echo Setting FM_BMC_DBP_PRESENT_R_N to 1 || echo Unable to set FM_BMC_DBP_PRESENT_R_N
gpioset $(gpiofind AP0_RST_IN_N)=1 >/dev/null 2>&1 && echo Setting AP0_RST_IN_N to 1 || echo Unable to set AP0_RST_IN_N
gpioset $(gpiofind BMC_REBOOT_PCH_N)=1 >/dev/null 2>&1 && echo Setting BMC_REBOOT_PCH_N to 1 || echo Unable to set BMC_REBOOT_PCH_N
gpioset $(gpiofind BMC_ADR_L)=0 >/dev/null 2>&1 && echo Setting BMC_ADR_L to 1 || echo Unable to set BMC_ADR_L
gpioset $(gpiofind BIOS_SEL_N)=1 >/dev/null 2>&1 && echo Setting BIOS_SEL_N to 1 || echo Unable to set BIOS_SEL_N
gpioset $(gpiofind PMBUS_SEL_N)=0 >/dev/null 2>&1 && echo Setting PMBUS_SEL_N to 0 || echo Unable to set BIOS_SEL_N

echo Finish to initialize BMC output GPIO
