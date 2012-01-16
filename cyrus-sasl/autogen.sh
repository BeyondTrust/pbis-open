#!/bin/sh

# From SMakefile in Cyrus SASL source repository

rm -f aclocal.m4
libtoolize -fic || exit $?
aclocal --force -I cmulocal -I config || exit $?
echo "cyrus after aclocal"
if type stat >/dev/null 2>&1; then
stat aclocal.m4
stat config.h.in
fi
autoheader -f || exit $?
echo "cyrus after autoheader"
if type stat >/dev/null 2>&1; then
stat aclocal.m4
stat config.h.in
fi
autoconf -f || exit $?
echo "cyrus after autoconf"
if type stat >/dev/null 2>&1; then
stat aclocal.m4
stat config.h.in
fi
automake --add-missing --include-deps || exit $?
echo "cyrus after automake"
if type stat >/dev/null 2>&1; then
stat aclocal.m4
stat config.h.in
fi

cd saslauthd || exit $?
rm -f aclocal.m4
libtoolize -fic || exit $?
aclocal --force -I ../cmulocal -I ../config || exit $?
echo "cyrus after aclocal2"
if type stat >/dev/null 2>&1; then
stat aclocal.m4
stat config.h.in
fi
autoheader -f || exit $?
echo "cyrus after autoheader2"
if type stat >/dev/null 2>&1; then
stat aclocal.m4
stat config.h.in
fi
autoconf -f || exit $?
echo "cyrus after autoconf2"
if type stat >/dev/null 2>&1; then
stat aclocal.m4
stat config.h.in
fi
automake --add-missing --include-deps || exit $?
echo "cyrus after automake2"
if type stat >/dev/null 2>&1; then
stat aclocal.m4
stat config.h.in
fi
