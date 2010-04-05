#!/bin/sh
./configure --enable-afnp --prefix=/opt/novell/xad --libdir=/opt/novell/xad/lib64 --sysconfdir=/etc/opt/novell/xad --localstatedir=/var/opt/novell/xad --with-pthread-ext-dir=/opt/novell
cp /usr/bin/libtool .
