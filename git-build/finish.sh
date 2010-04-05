#!/bin/bash

AFTER_CMD="rsync -a -K -v --exclude \"*.la\" staging/install-root/ <root-of-target-filesystem>/"

case "$1" in
    redhat)
	;;
    suse)
	;;
    none)
	;;
    *)
	echo "usage: $0 <redhat> | <suse> | <none>"
	echo ""
	echo "Afterwards: ${AFTER_CMD}"
	exit 1
	;;
esac

set -e

find staging/install-root -name .libs -exec rm {} \;
cp config/init-base.sh staging/install-root/opt/likewise/bin/.
cp config/likewise-krb5-ad.conf staging/install-root/etc/likewise/.
cp config/krb5.conf.default staging/install-root/etc/likewise/krb5.conf

for a in \
    etc/init.d \
    var/lib/likewise/run \
    opt/likewise/data \
    ; do
    if [ ! -d staging/install-root/${a} ]; then
	mkdir staging/install-root/${a}
    fi
done

for a in \
    eventlogd \
    dcerpcd \
    lsassd \
    lwiod \
    netlogond \
    srvsvcd \
    ; do
    sed \
        -e s:PREFIX_DIR:/opt/likewise: \
	-e s:EXECDIR:/opt/likewise/bin: \
        -e s:#LWI_STARTUP_TYPE_REDHAT:: \
	config/${a} > staging/install-root/etc/init.d/${a}
    chmod 775 staging/install-root/etc/init.d/${a}
done

echo "Now just do: ${AFTER_CMD}"
