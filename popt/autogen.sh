#!/bin/sh

srcdir="`dirname $0`"
test -z "$srcdir" && srcdir=.

THEDIR="`pwd`"

libtoolize=`which glibtoolize 2>/dev/null`
case $libtoolize in
/*) ;;
*)  libtoolize=`which libtoolize 2>/dev/null`
    case $libtoolize in
    /*) ;;
    *)  libtoolize=libtoolize
    esac
esac

cd "$srcdir"
$libtoolize --copy --force
gettextize --copy --force --no-changelog
perl -p -i~ -e 's/(po\/Makefile\.in)\s+po\/Makefile\.in/$1/' configure.ac
perl -p -i~ -e 's/(SUBDIRS\s+=\s+po)\s+po/$1/' Makefile.am
aclocal -I m4
autoheader
automake -a -c
autoconf

if [ "$1" = "--noconfigure" ]; then 
    exit 0;
fi

cd "$THEDIR"

if [ X"$@" = X  -a "X`uname -s`" = "XLinux" ]; then
    $srcdir/configure --prefix=/usr --libdir=/lib "$@"
else
    $srcdir/configure "$@"
fi
