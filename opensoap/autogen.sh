#! /bin/sh
set -x
rm -rf autom4te.cache/;
rm -f aclocal.m4;
if test ! -d config; then mkdir config; fi
#aclocal  --force -I config || exit $?
#libtoolize --force --copy
libtoolize -fic || exit $?
aclocal  --force || exit $?
autoheader
automake --foreign --add-missing --copy
autoconf
