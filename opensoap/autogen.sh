#! /bin/sh
set -x
rm -rf autom4te.cache/;
rm -f aclocal.m4;
chmod 666 configure.h.in
if test ! -d config; then mkdir config; fi
#aclocal  --force -I config || exit $?
#libtoolize --force --copy
libtoolize -f -c || exit $?
aclocal  --force || exit $?
autoheader -f || exit $?
automake --foreign --add-missing --copy
autoconf -f  || exit $?
chmod -R 777 autom4te.cache
