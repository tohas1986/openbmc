SUMMARY = "BMC Assisted Fru Isolation"
DESCRIPTION = "Isolates failing FRU from crashdump."
LICENSE = "CLOSED"
SRCREV = "ddde989c1fb997eeb684cb1f6d222cab85360394"
PV = "1.0+git${SRCPV}"
SRC_URI = "git://github.com/Intel-BMC/bafi;protocol=https;branch=master"
S = "${WORKDIR}/git"
DEPENDS = "nlohmann-json"

do_install() {

        install -D -m 0644 ${S}/include/utils.hpp ${D}${includedir}/bafi/utils.hpp
        install -D -m 0644 ${S}/include/aer.hpp ${D}${includedir}/bafi/aer.hpp
        install -D -m 0644 ${S}/include/tor_defs.hpp ${D}${includedir}/bafi/tor_defs.hpp
        install -D -m 0644 ${S}/include/tor_defs_icx.hpp ${D}${includedir}/bafi/tor_defs_icx.hpp
        install -D -m 0644 ${S}/include/tor_defs_cpx.hpp ${D}${includedir}/bafi/tor_defs_cpx.hpp
        install -D -m 0644 ${S}/include/tor_defs_skx.hpp ${D}${includedir}/bafi/tor_defs_skx.hpp
        install -D -m 0644 ${S}/include/tor_defs_spr.hpp ${D}${includedir}/bafi/tor_defs_spr.hpp
        install -D -m 0644 ${S}/include/mca_defs.hpp ${D}${includedir}/bafi/mca_defs.hpp
        install -D -m 0644 ${S}/include/mca_icx.hpp ${D}${includedir}/bafi/mca_icx.hpp
        install -D -m 0644 ${S}/include/mca_cpx.hpp ${D}${includedir}/bafi/mca_cpx.hpp
        install -D -m 0644 ${S}/include/mca_skx.hpp ${D}${includedir}/bafi/mca_skx.hpp
        install -D -m 0644 ${S}/include/mca_spr.hpp ${D}${includedir}/bafi/mca_spr.hpp
        install -D -m 0644 ${S}/include/cpu.hpp ${D}${includedir}/bafi/cpu.hpp
        install -D -m 0644 ${S}/include/cpu_factory.hpp ${D}${includedir}/bafi/cpu_factory.hpp
        install -D -m 0644 ${S}/include/tor_whitley.hpp ${D}${includedir}/bafi/tor_whitley.hpp
        install -D -m 0644 ${S}/include/tor_purley.hpp ${D}${includedir}/bafi/tor_purley.hpp
        install -D -m 0644 ${S}/include/tor_eaglestream.hpp ${D}${includedir}/bafi/tor_eaglestream.hpp
        install -D -m 0644 ${S}/include/tor_icx.hpp ${D}${includedir}/bafi/tor_icx.hpp
        install -D -m 0644 ${S}/include/tor_cpx.hpp ${D}${includedir}/bafi/tor_cpx.hpp
        install -D -m 0644 ${S}/include/tor_skx.hpp ${D}${includedir}/bafi/tor_skx.hpp
        install -D -m 0644 ${S}/include/tor_spr.hpp ${D}${includedir}/bafi/tor_spr.hpp
        install -D -m 0644 ${S}/include/pcilookup.hpp ${D}${includedir}/bafi/pcilookup.hpp
        install -D -m 0644 ${S}/include/mca.hpp ${D}${includedir}/bafi/mca.hpp
        install -D -m 0644 ${S}/include/summary.hpp ${D}${includedir}/bafi/summary.hpp
        install -D -m 0644 ${S}/include/generic_report.hpp ${D}${includedir}/bafi/generic_report.hpp
        install -D -m 0644 ${S}/include/report.hpp ${D}${includedir}/bafi/report.hpp
        install -D -m 0644 ${S}/include/triage.hpp ${D}${includedir}/bafi/triage.hpp
}
