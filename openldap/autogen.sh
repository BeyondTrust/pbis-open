rm -rf autom4te.cache/;
(aclocal --force && libtoolize -fic && autoconf -f && autoheader -f) || exit 1
rm -rf autom4te.cache/
