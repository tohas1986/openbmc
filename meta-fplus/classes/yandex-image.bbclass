inherit image_types_phosphor yandex-version

SSTATE_ALLOW_OVERLAP_FILES = "/"

PSEUDO_IGNORE_PATHS:append = "/rwfs,${PSEUDO_SYSROOT}/rwfs,${IMAGE_ROOTFS}/rwfs,${WORKDIR}/rwfs,rwfs"

do_rootfs[network] = "1"
do_generate_static_alltar[network] = "1"

make_signatures[network] = "1"
make_signatures() {
    bbplain "Yandex debug: make_signatures for ${IMAGE_NAME}"
    signature_files=""
	for file in "$@"; do
		openssl dgst -sha256 -sign ${SIGNING_KEY} -out "${file}.sig" $file
		signature_files="${signature_files} ${file}.sig"
	done

	if [ -n "$signature_files" ]; then
		sort_signature_files=`echo "$signature_files" | tr ' ' '\n' | sort | tr '\n' ' '`
		cat $sort_signature_files > image-full
		openssl dgst -sha256 -sign ${SIGNING_KEY} -out image-full.sig image-full
		signature_files="${signature_files} image-full.sig"
	fi
    #signature_files=""
    #export SSH_AUTH_SOCK=${SSH_AUTH_SOCK}
    #pf=$(mktemp)
    #TODO: Check this and refactor
    #/usr/bin/yav get version sec-01csy97rg46f9d5z2cpzm0nrge -O priv > $pf
    #for file in "$@"; do
    #    bbplain "Yandex debug: Signing ${file}"
    #    openssl dgst -sha256 -sign $pf -out "${file}.sig" $file || :
    #    signature_files="${signature_files} ${file}.sig" || :
    #done
    #rm -rf $pf >/dev/null 2>&1
    bbplain "Yandex debug: make_signatures for ${IMAGE_NAME} done"
}

final_cleanup() {
    bbplain "Yandex debug: Doing final cleanup for ${IMAGE_NAME}"
    #find ${IMAGE_ROOTFS} -name "xyz.openbmc_project.Ldap.Config.service*" -exec rm -rf {} \; || :
    find ${IMAGE_ROOTFS} -name "NVME*P4000.json" -exec rm -rf {} \; || :
    find ${IMAGE_ROOTFS} -name "*core*dump*" -exec rm -rf {} \; || :
    rm -rf ${IMAGE_ROOTFS}/log_lock.pid >/dev/null 2>&1 || :
    find ${IMAGE_ROOTFS} -name "*obmc-read-eeprom*" -exec rm -rf {} \; || :
    #find ${IMAGE_ROOTFS} -name "*Inventory.Manager*" -exec rm -rf {} \; || :
    #find ${IMAGE_ROOTFS} -name "*phosphor-inventory*" -exec rm -rf {} \; || :
    find ${IMAGE_ROOTFS} -name "*systemd-timesyncd*" -exec rm -rf {} \; || :
    find ${IMAGE_ROOTFS} -name "*obmc-fru-fault-monitor*" -exec rm -rf {} \; || :
    find ${IMAGE_ROOTFS} -name "*phosphor-fru-fault-monitor*" -exec rm -rf {} \; || :
    find ${IMAGE_ROOTFS} -name "*bmc_booted*" -exec rm -rf {} \; || :
    find ${IMAGE_ROOTFS} -name "*phosphor-dbus-monitor*" -exec rm -rf {} \; || :
    find ${IMAGE_ROOTFS} -name "*Dump.Manager*" -exec rm -rf {} \; || :
    find ${IMAGE_ROOTFS} -name "nscd.service" -exec rm -rf {} \; || :
    find ${IMAGE_ROOTFS} -name "sysfssensor" -exec rm -rf {} \; || :

    rm -rf ${IMAGE_ROOTFS}/lib/udev/rules.d/70-iio.rules  >/dev/null 2>&1 || :
    rm -rf ${IMAGE_ROOTFS}/usr/share/entity-manager/configurations/blocklist.json >/dev/null 2>&1 || :

    sed -i '/pam_cracklib.so/d' ${IMAGE_ROOTFS}/etc/pam.d/common-password || :
    sed -i 's/pam_ipmicheck.so spec_grp_name=ipmi use_authtok/pam_ipmicheck.so spec_grp_name=ipmi/g' ${IMAGE_ROOTFS}/etc/pam.d/common-password || :

    mkdir ${IMAGE_ROOTFS}/rwfs || :
    date > ${IMAGE_ROOTFS}/rwfs/timestamp

    mkdir ${IMAGE_ROOTFS}/persistent || :
    bbplain "Yandex debug: Final cleanup for ${IMAGE_NAME} done"
}

update_version[network] = "1"
update_version() {
    bbplain "Yandex debug: update_version for ${IMAGE_NAME}"

    ##################################################################
    # Date/time variables.
    ##################################################################

    date="${@time.strftime('%Y%m%d',time.gmtime())}"
    time="${@time.strftime('%H%M%S',time.gmtime())}"
    datetime="${date}${time}"

    # Get VERSION_ID from os-release and split it
    deb=$(cat ${IMAGE_ROOTFS}/etc/os-release)
    bbplain "\n os-releas \n ${deb} \n"
    verOY="13407331"
    ver=$(cat ${IMAGE_ROOTFS}/etc/os-release | grep VERSION_ID | awk -F "=" '{print $2}')
    #add FIXED_Y_VERSION to VERSION_ID
    sed -i "s/VERSION_ID=/VERSION_ID=${FIXED_Y_VERSION}-/g" ${IMAGE_ROOTFS}/etc/os-release
    echo "VERSION=\"${FIXED_Y_VERSION}\"" >> ${IMAGE_ROOTFS}/etc/os-release
    VERSION_ID="${FIXED_Y_VERSION}-${verOY}"
    echo "VERSION_ID=\"${VERSION_ID}\"" >> ${IMAGE_ROOTFS}/etc/os-release
    echo "EXTENDED_VERSION=\"${VERSION_ID}\"" >> ${IMAGE_ROOTFS}/etc/os-release
    #replace revision or hash into VERSION on FIXED_Y_VERSION
    sed -i "s/VERSION=\"${ver}\"/VERSION=\"${FIXED_Y_VERSION}\"/g" ${IMAGE_ROOTFS}/etc/os-release
    sed -i "s/EXTENDED_VERSION=\"${FIXED_Y_VERSION}\"/EXTENDED_VERSION=\"${FIXED_Y_VERSION}-${ver}\"/g" ${IMAGE_ROOTFS}/etc/os-release
    echo "YANDEX_SHORT_VER=\"${FIXED_Y_VERSION}\"" >> ${IMAGE_ROOTFS}/etc/os-release
    echo "CHECK_ID=\"${DATETIME}\"" >> ${IMAGE_ROOTFS}/etc/os-release
    deb=$(cat ${IMAGE_ROOTFS}/etc/os-release)
    bbplain "\n os-releas \n ${deb} \n"
    bbplain "Yandex debug: Update os-release in ${IMAGE_NAME} to ver. ${FIXED_Y_VERSION}"
}

update_issue() {
    bbplain "Yandex debug: update_issue for ${IMAGE_NAME}"
    ver_id=$(cat ${IMAGE_ROOTFS}/etc/os-release | grep VERSION_ID | awk -F "=" '{print $2}')
    printf "Yandex OpenBMC (Based on Phosphor OpenBMC Project) $ver_id \\\n \\\l\n" > ${IMAGE_ROOTFS}/etc/issue
    echo "Yandex OpenBMC (Based on Phosphor OpenBMC Project) $ver_id %h" > ${IMAGE_ROOTFS}/etc/issue.net
    bbplain "Yandex debug: update_issue for ${IMAGE_NAME} done"
}

set_root_password[network] = "0"
set_root_password() {
    bbplain "Yandex debug: Setting root password in ${IMAGE_NAME}"
    export SSH_AUTH_SOCK=${SSH_AUTH_SOCK}

    #TODO: Check this and refactor
    #FIXME hash=$(/usr/bin/yav get version sec-01cv2743z427ze0x9482mx1qs2 -O root_hash)
    #hash=$(/usr/bin/yav get version sec-01cv2743z427ze0x9482mx1qs2 -O strong_hash)
    #usermod --root ${IMAGE_ROOTFS} -p ${hash} root
    #hash=$(/usr/bin/yav get version sec-01cv2743z427ze0x9482mx1qs2 -O admin_hash)
    #usermod --root ${IMAGE_ROOTFS} -p ${hash} admin

    usermod --root ${IMAGE_ROOTFS} -p '$6$KlbMjZb1ZADql84L$oX1SzeDOkCH.6TZGc2jXHZHbhqX1o8rllXb.achwH4XT0gl7YCYN0QMSanyjRQPSD2fyOuS9riA2SYtJBHj2E/' root
    mkdir -p ${IMAGE_ROOTFS}/home/root/.ssh
    #ssh_pub=$(/usr/bin/yav get version sec-01cv2743z427ze0x9482mx1qs2 -O ssh_pub)
    echo $ssh_pub > ${IMAGE_ROOTFS}/home/root/.ssh/authorized_keys
    chmod 0600 ${IMAGE_ROOTFS}/home/root/.ssh/authorized_keys
    bbplain "Yandex debug: Setting root password in ${IMAGE_NAME} done"
}

install_yandex_cert() {
    if [ -f "${IMAGE_ROOTFS}/usr/share/ca-certificates/yandex/allCAs.crt" ]; then
        bbplain "Yandex debug: Installing Yandex certificate"
        cp ${IMAGE_ROOTFS}/usr/share/ca-certificates/yandex/allCAs.crt ${IMAGE_ROOTFS}/etc/ssl/certs/ca-certificates.crt
    else
        bbplain "Yandex debug: External image, no Yandex cert is installed"
    fi
}

# Need to make rwfs archive and put it to rofs.
# It contains dropbear configuration, simple shadow, default ipmi_pass.
# It's used for factory_reset.
make_rwfs_archive[network] = "1"
make_rwfs_archive() {
    bbplain "Yandex debug: Creating rwfs golden archive"
    rwdir=$(pwd)/jffs2
    rm -rf $rwdir > /dev/null 2>&1 || :
    mkdir $rwdir

    cp -a ${IMAGE_ROOTFS}/rwfs/* ${rwdir}/ || :
    cp -a ${IMAGE_ROOTFS}/etc/passwd ${IMAGE_ROOTFS}/etc/group ${IMAGE_ROOTFS}/etc/shadow ${IMAGE_ROOTFS}/etc/gshadow ${IMAGE_ROOTFS}/etc/ipmi_pass ${rwdir}/etc/
    # Get simple root password from vault
    # export SSH_AUTH_SOCK=${SSH_AUTH_SOCK}
    # hash=$(/usr/bin/yav get version sec-01cv2743z427ze0x9482mx1qs2 -O easy_hash)
    # sed -i 's|^\(root:\)[^:]*\(:.*\)$|\1'${hash}'\2|g' ${rwdir}/etc/shadow

    # Create archive right in the ROOTFS
    mkdir -p ${IMAGE_ROOTFS}/usr/share/rwfs/
    tar czf ${IMAGE_ROOTFS}/usr/share/rwfs/rwfs.tgz --owner=0 --group=0 -C ${rwdir}/ .
    rm -rf ${IMAGE_ROOTFS}/rwfs $rwdir || :
}

ROOTFS_POSTPROCESS_COMMAND += "final_cleanup; update_version; update_issue; set_root_password; install_yandex_cert; make_rwfs_archive;"
#ROOTFS_POSTPROCESS_COMMAND += "final_cleanup; update_version; update_issue; install_yandex_cert; make_rwfs_archive;"
OVERLAY_MKFS_OPTS = " --pad=${RWFS_SIZE}"

# Generate file with ROFS hash. Sign it.
do_generate_rofs_signature[network] = "1"
do_generate_rofs_signature() {
    bbplain "Yandex debug: Calcualting critical ROFS files signature"
    squash_size=$(stat -c '%s' ${IMGDEPLOYDIR}/${IMAGE_NAME}${IMAGE_NAME_SUFFIX}.${IMAGE_BASETYPE})
    hash=$(cat ${IMGDEPLOYDIR}/${IMAGE_NAME}${IMAGE_NAME_SUFFIX}.${IMAGE_BASETYPE} | sha256sum -b )

    rwdir=${S}/static/jffs2
    bbplain "Yandex debug: RWDIR: ${rwdir}"
    rm -rf $rwdir > /dev/null 2>&1
    mkdir -p ${rwdir}
    echo "$squash_size $hash" > ${rwdir}/rofs_hash.txt

    pf=$(mktemp)
    export SSH_AUTH_SOCK=${SSH_AUTH_SOCK}
    #TODO: Check this and refactor
    #/usr/bin/yav get version sec-01csy97rg46f9d5z2cpzm0nrge -O priv > $pf
    #openssl dgst -sha256 -sign $pf -out ${rwdir}/rofs_hash.sig ${rwdir}/rofs_hash.txt
    #rm -rf $pf || :
}

# Override JFFS image creation
do_generate_rwfs_static[network] = "1"
do_generate_rwfs_static() {
    bbplain "Yandex debug: do_generate_rwfs_static in ${IMAGE_NAME}"

    rwdir=${S}/static/jffs2
    mkdir -p ${rwdir}/cow
    tar xzvf ${IMAGE_ROOTFS}/usr/share/rwfs/rwfs.tgz -C $rwdir/cow

    ${JFFS2_RWFS_CMD} ${OVERLAY_MKFS_OPTS} ${OVERLAY_MKFS_OPTS} --squash-uids
}

do_generate_static_tar[network] = "1"
do_generate_static_tar:prepend() {
    bbplain "Yandex debug: Prepend to generate_static_tar for ${IMAGE_NAME}"
}

do_generate_static_tar:append() {
    bbplain "Yandex debug: Append to generate_static_tar for ${IMAGE_NAME}"
    sums="checksums.txt"
    now=$(date "+%Y-%m-%d-%H-%M")
    bbplain "Yandex debug: Processing upgrade image ${IMAGE_NAME}"

    rm ${IMGDEPLOYDIR}/${IMAGE_NAME}.upgrade*tar 2>/dev/null || :
    echo > $sums
    tar tf ${IMAGE_NAME}.static.mtd.tar | while read f; do
        [ "$f" = "$sums" ] && continue
        sum=$(tar xfO ${IMAGE_NAME}.static.mtd.tar $f | md5sum)
        echo $f $sum >> $sums
    done
    tar rf ${IMAGE_NAME}.static.mtd.tar $sums
    cp ${IMAGE_NAME}.static.mtd.tar ${IMGDEPLOYDIR}/${IMAGE_NAME}.upgrade.${FIXED_Y_VERSION}.$now.tar
    rm $sums || :
}

do_generate_static_alltar:append() {
    bbplain "Yandex debug: Append to generate_static_alltar for ${IMAGE_NAME}"
    sums="checksums.txt"
    now=$(date "+%Y-%m-%d-%H-%M")
    bbplain "Yandex debug: Processing fullflash image ${IMAGE_NAME}"

    rm ${IMGDEPLOYDIR}/${IMAGE_NAME}.fullflash*tar 2>/dev/null || :
    echo > $sums
    tar tf ${IMAGE_NAME}.static.mtd.all.tar | while read f; do
        [ "$f" = "$sums" ] && continue
        sum=$(tar xfO ${IMAGE_NAME}.static.mtd.all.tar $f | md5sum)
        echo $f $sum >> $sums
    done
    bbplain "Yandex debug: fullflash: Done calculating checksums"
    tar rf ${IMAGE_NAME}.static.mtd.all.tar $sums
    cp ${IMAGE_NAME}.static.mtd.all.tar ${IMGDEPLOYDIR}/${IMAGE_NAME}.fullflash.${FIXED_Y_VERSION}.$now.tar
    rm $sums || :
    bbplain "Yandex debug: fullflash: Done all tasks"
}

python do_generate_phosphor_manifest() {
    purpose = d.getVar('VERSION_PURPOSE', True)
    #version = do_get_version(d)
    version = "2.28-12345678"
    target_machine = d.getVar('MACHINE', True)
    extended_version = (d.getVar('EXTENDED_VERSION', True) or "")
    yandex_version = d.getVar('FIXED_Y_VERSION', True)
    with open('MANIFEST', 'w') as fd:
        fd.write('purpose={}\n'.format(purpose))
        fd.write('version={}\n'.format(version.strip('"')))
        fd.write('ExtendedVersion={}\n'.format(extended_version))
        fd.write('KeyType={}\n'.format(get_pubkey_type(d)))
        fd.write('HashType=RSA-SHA256\n')
        fd.write('MachineName={}\n'.format(target_machine))
        fd.write('YandexVersion={}\n'.format(yandex_version))
}

addtask do_generate_rofs_signature after do_image_squashfs_xz before do_generate_rwfs_static
