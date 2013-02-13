#!/bin/sh

WORK_ROOT_DIR=/home/snambakam/workspaces/Sapphire.meta/linux/i386/components
OPENLDAP_DIR=$WORK_ROOT_DIR/openldap/staging/root/usr/centeris
EVENTLOG_DIR=$WORK_ROOT_DIR/eventlog/staging/root/usr/centeris
DCERPC_DIR=$WORK_ROOT_DIR/dcerpc/staging/root/usr/centeris

if [ -f ./Makefile ]; then
make distclean
fi

./autogen.sh

./configure --with-openldap-dir=$OPENLDAP_DIR \
            --with-eventlog-dir=$EVENTLOG_DIR \
            --with-dce-dir=$DCERPC_DIR        \
            --enable-eventlog=no              \
            --enable-debug

