do_install[network] = "1"
do_install() {
    #bbplain "DEBUG: Fetching public key from YAV"
    #export SSH_AUTH_SOCK=${SSH_AUTH_SOCK}
    #/usr/bin/yav get version sec-01csy97rg46f9d5z2cpzm0nrge -O pub > ${WORKDIR}/publickey
    openssl pkey -in "${SIGNING_KEY}" -pubout -out ${WORKDIR}/publickey
    echo HashType=RSA-SHA256 > "${WORKDIR}/hashfunc"

    idir="${D}${sysconfdir}/activationdata/${SIGNING_KEY_TYPE}"
    bbplain "DEBUG: Saved publickey"

    install -d ${idir}
    install -m 644 ${WORKDIR}/publickey ${idir}
    install -m 644 ${WORKDIR}/hashfunc ${idir}
}

do_install[network] = "1"
