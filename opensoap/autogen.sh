#! /bin/sh

set -x
if test ! -d config; then mkdir config; fi
aclocal  --force -I config || exit $?
#libtoolize --force --copy
libtoolize -fic || exit $?
autoheader
automake --foreign --add-missing --copy
autoconf
