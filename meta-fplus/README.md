# Yandex OpenBmc port to Yandex platfroms


## Current OpenBMC commit:
```
commit 8f7e767789a4e5da9ad218853ff6e61d395526ea (HEAD -> master, origin/master, origin/HEAD)
Author: Andrew Geissler <openbmcbump-github@yahoo.com>
Date:   Thu Dec 8 12:50:32 2022 -0600

    phosphor-networkd: srcrev bump 9bc50f5e6b..a4c18d4e50
    
    Patrick Williams (2):
          prettier: re-format
          beautysh: re-format
    
    Change-Id: I6fdd55bc093a5890297f191de830cc001903ec4a
    Signed-off-by: Andrew Geissler <openbmcbump-github@yahoo.com>
```

## Platforms list:
- MY81
- MZ81
- MY62
- MZB2
- AI1
- GD1
- G1
- SOM2600v1 (Ampere MB)


# Building instructions:

## Build is linked to absolute path on disk:
```
mkdir -p /work/obmc_downloads
chmod 777 /work/obmc_downloads
mkdir -p /work/openbmc_fw
cd /work
git clone https://github.com/openbmc/openbmc/
cd openbmc
git clone ssh://git@bb.yandex-team.ru/rnd/yandex-openbmc.git
mv yandex-openbmc meta-yandex
```

## Yandex meta is linked to specific upstream commit indicated above
```
git reset --hard $(cat meta-yandex/README.md  | grep "^commit" | awk '{print $2}')
```

## Build for one machine, result is in build/\<machine\>/tmp/deploy/images/\<machine\>/\<machine\>.*.tar
```
source setup my81
bitbake image-ext image-yandex
ls -al tmp/deploy/images/my81/my81.*.tar
```

## Build for all machines, result is in /work/openbmc_fw
```
cd /work/openbmc/meta-yandex
./buildall.sh
ls -al /work/openbmc_fw/
```
