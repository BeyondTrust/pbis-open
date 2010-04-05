#!/bin/sh
CFLAGS="-march=i686" \
CPPFLAGS="-march=i686" \
./configure --enable-afnp --prefix=/opt/novell/xad --sysconfdir=/etc/opt/novell/xad --localstatedir=/var/opt/novell/xad --with-pthread-ext-dir=/opt/novell
cp /usr/bin/libtool .

