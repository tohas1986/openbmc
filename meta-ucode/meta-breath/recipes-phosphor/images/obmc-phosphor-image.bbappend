# The service account needs sudo.
# IMAGE_INSTALL:append = " ${@bb.utils.contains('DISTRO_FEATURES', 'ibm-service-account-policy', 'sudo', '', d)}"
# IMAGE_INSTALL:append = " phosphor-skeleton-control-power "
