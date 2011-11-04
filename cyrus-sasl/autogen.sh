#!/bin/sh

# From SMakefile in Cyrus SASL source repository

rm -f aclocal.m4
libtoolize -fic || exit $?
aclocal --force -I cmulocal -I config || exit $?
autoheader -f || exit $?
autoconf -f || exit $?
automake --add-missing --include-deps || exit $?

cd saslauthd || exit $?
rm -f aclocal.m4
libtoolize -fic || exit $?
aclocal --force -I ../cmulocal -I ../config || exit $?
autoheader -f || exit $?
autoconf -f || exit $?
automake --add-missing --include-deps || exit $?
