FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

# Remove, from the p10bmc image, the service file that starts the skeleton power
# control application. That image will use the power control application
# included in the phosphor-power repository.

# OBMC_CONTROL_FMT = ""
# FILES:${PN} += " /usr/lib/systemd/system/multi-user.target.wants/org.openbmc.control.Power@0.service "
