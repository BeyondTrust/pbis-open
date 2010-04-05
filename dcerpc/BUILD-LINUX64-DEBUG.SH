#!/bin/sh
CFLAGS="-D_GNU_SOURCE -DRPC_MUTEX_DEBUG -DRPC_MUTEX_STATS" \
CPPFLAGS="-D_GNU_SOURCE -DRPC_MUTEX_DEBUG -DRPC_MUTEX_STATS" \
./configure --enable-afnp --prefix=/opt/novell/xad --libdir=/opt/novell/xad/lib64 --with-pthread-ext-dir=/opt/novell
cp /usr/bin/libtool .
