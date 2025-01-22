FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-Be-more-verbose-in-case-of-size-error.patch \
            file://0002-Fix-bug-with-missing-value_start.patch \
            file://0003-Mark-as-last-only-16th-key-in-data-area.patch \
            "
