# m4.m4 serial 2
dnl Copyright (C) 2000, 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# AC_PROG_GNU_M4
# --------------
# Check for GNU m4, at least 1.3 (supports frozen files).
# Also, check whether --error-output (through 1.4.x) or --debugfile (2.0)
# is supported, and AC_SUBST M4_DEBUGFILE accordingly.
AC_DEFUN([AC_PROG_GNU_M4],
[AC_PATH_PROGS([M4], [gm4 gnum4 m4], [m4])
AC_CACHE_CHECK([whether m4 supports frozen files], [ac_cv_prog_gnu_m4],
[ac_cv_prog_gnu_m4=no
if test x"$M4" != x; then
  case `$M4 --help < /dev/null 2>&1` in
    *reload-state*) ac_cv_prog_gnu_m4=yes ;;
  esac
fi])
if test $ac_cv_prog_gnu_m4 = yes ; then
  AC_CACHE_CHECK([how m4 supports trace files], [ac_cv_prog_gnu_m4_debugfile],
  [case `$M4 --help < /dev/null 2>&1` in
    *debugfile*) ac_cv_prog_gnu_m4_debugfile=--debugfile ;;
    *) ac_cv_prog_gnu_m4_debugfile=--error-output ;;
  esac])
  AC_SUBST([M4_DEBUGFILE], $ac_cv_prog_gnu_m4_debugfile)
fi
])
