rm -rf autom4te.cache/;
(aclocal --force && libtoolize -f -c && autoconf -f && autoheader -f) || exit 1
autoreconf -i
rm -rf autom4te.cache/
