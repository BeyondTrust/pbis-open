dnl find the tdb sources. This is meant to work both for
dnl tdb standalone builds, and builds of packages using tdb
tdbdir=""
tdbpaths="$srcdir $srcdir/lib/tdb $srcdir/tdb $srcdir/../tdb"
for d in $tdbpaths; do
        if test -f "$d/common/tdb.c"; then
                tdbdir="$d"
                AC_SUBST(tdbdir)
                break;
        fi
done
if test x"$tdbdir" = "x"; then
   AC_MSG_ERROR([cannot find tdb source in $tdbpaths])
fi

libreplacedir=$srcdir/replace
AC_SUBST(libreplacedir)

AC_MSG_CHECKING([for Python])

PYTHON=
 
AC_ARG_WITH(python,
[  --with-python=PYTHONNAME  build Python libraries],
[ case "${withval-python}" in
  yes)
        PYTHON=python
        ;;
  no)
        PYTHON=
        ;;
  *)
        PYTHON=${withval-python}
        ;;
  esac ])

if test x"$PYTHON" != "x"; then
	incdir=`python -c 'import sys; print "%s/include/python%d.%d" % (sys.prefix, sys.version_info[[0]], sys.version_info[[1]])'`
	CPPFLAGS="$CPPFLAGS -I $incdir"
fi

if test x"$PYTHON" != "x"; then
	AC_MSG_RESULT([${withval-python}])
else
	SMB_ENABLE(swig_tdb, NO)
	AC_MSG_RESULT(no)
fi

AC_SUBST(PYTHON)
