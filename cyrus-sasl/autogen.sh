#!/bin/sh

# From SMakefile in Cyrus SASL source repository

rm -f aclocal.m4
libtoolize -c -f || exit $?
aclocal -I cmulocal -I config || exit $?
autoheader || exit $?
autoconf || exit $?
automake --add-missing --include-deps || exit $?

cd saslauthd || exit $?
rm -f aclocal.m4
libtoolize -c -f || exit $?
aclocal -I ../cmulocal -I ../config || exit $?
autoheader || exit $?
autoconf || exit $?
automake --add-missing --include-deps || exit $?
