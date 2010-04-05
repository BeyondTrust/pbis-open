#!/bin/sh
CFLAGS="-march=i686 -DRPC_MUTEX_DEBUG -DRPC_MUTEX_STATS" \
CPPFLAGS="-march=i686 -DRPC_MUTEX_DEBUG -DRPC_MUTEX_STATS" \
./configure --enable-afnp --prefix=/opt/novell/xad --sysconfdir=/etc/opt/novell/xad --localstatedir=/var/opt/novell/xad --with-pthread-ext-dir=/opt/novell
cp /usr/bin/libtool .
