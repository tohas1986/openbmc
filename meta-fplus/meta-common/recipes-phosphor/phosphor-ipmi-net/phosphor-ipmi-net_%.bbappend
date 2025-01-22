FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0002-Add-the-max-size-limit-feature-to-SOL-Console-data.patch"
SRC_URI += "file://0012-Return-insecure-algos.patch"
SRC_URI += "file://0014-Remove-PAM-call.patch"
SRC_URI += "file://0017-Set-StartLimitBurst.patch"
SRC_URI += "file://0018-Fix-SEGV-Reduce-accumulation-interval-for-SOL.patch"

CXXFLAGS:append = " -DRMCP_PING "
