_LW_DOXYGEN_DISABLED=""
_LW_DOXYGEN_PFD_DISABLED=""
dnl
dnl _LW_UPDATE_HAVE_DOXYGEN
dnl
dnl Update HAVE_DOXYGEN and HAVE_DOXYGEN_PDF to true/false based on current
dnl state.
dnl
AC_DEFUN([_LW_UPDATE_HAVE_DOXYGEN],[
    AS_IF([test -n "$DOXYGEN" -a -z "$_LW_DOXYGEN_DISABLED"],[
        HAVE_DOXYGEN=true
    ],[
        HAVE_DOXYGEN=false
    ])
    AS_IF([test -n "$DOXYGEN" -a -n "$PDFLATEX" -a -z "$_LW_DOXYGEN_PDF_DISABLED" -a "x$HAVE_DOXYGEN" = "xtrue" ],[
        HAVE_DOXYGEN_PDF=true
    ],[
        HAVE_DOXYGEN_PDF=false
    ])
])
dnl
dnl LW_PROG_DOXYGEN
dnl
dnl This works like AC_PROG_SED, etc. by defining DOXYGEN as either
dnl doxygen or "" but also sets HAVE_DOXYGEN to true or false.
dnl
AC_DEFUN([LW_PROG_DOXYGEN],[
    AC_CHECK_PROG([DOXYGEN], [doxygen], [doxygen], [])
    _LW_UPDATE_HAVE_DOXYGEN
])
dnl
dnl LW_PROG_DOXYGEN_MIN_VERSION([MIN-VERSION])
dnl 
dnl This works like LW_PROG_DOXYGEN but requires a minimum doxygen version.
dnl
dnl NOTE: Requires AX_COMPARE_VERSION from
dnl       http://autoconf-archive.cryp.to/ax_compare_version.html.
dnl
AC_DEFUN([LW_PROG_DOXYGEN_MIN_VERSION],[
    AC_REQUIRE([LW_PROG_DOXYGEN])
    AS_IF([test -n "$DOXYGEN"],[
        AC_MSG_CHECKING([doxygen version])
        _lw_doxygen_version=`$DOXYGEN --version 2>&1`
        AC_MSG_RESULT($_lw_doxygen_version)
        AX_COMPARE_VERSION([$_lw_doxygen_version],[lt],[$1],[
            AC_MSG_WARN([ignoring doxygen version less than $1])
            DOXYGEN=""
            _LW_UPDATE_HAVE_DOXYGEN
        ])
    ],[
        AC_MSG_WARN([doxygen is not present])
    ])
])
dnl
dnl LW_PROG_PDFLATEX
dnl
dnl This works like AC_PROG_SED, etc. by defining PDFLATEX as either
dnl doxygen or "" but also sets HAVE_PDFLATEX to true or false.
dnl
AC_DEFUN([LW_PROG_PDFLATEX],[
    AC_CHECK_PROG([PDFLATEX], [pdflatex], [pdflatex], [])
    _LW_UPDATE_HAVE_DOXYGEN
])
dnl
dnl LW_USE_DOXYGEN
dnl
AC_DEFUN([LW_USE_DOXYGEN],[
    AC_REQUIRE([LW_PROG_DOXYGEN])
    AC_REQUIRE([LW_PROG_PDFLATEX])
    AS_IF([test -n "$1"],[
        LW_PROG_DOXYGEN_MIN_VERSION([$1])
    ])
    AC_ARG_ENABLE([doxygen],[AS_HELP_STRING([--enable-doxygen],[enable doxygen])],[
        AS_IF([test "x$enable_doxygen" != "xno"],[
            AS_IF([test "x$HAVE_DOXYGEN" = "xfalse"],[
                AC_MSG_FAILURE([Cannot enable doxygen due to missing dependencies.])
            ])
        ],[
             AC_MSG_NOTICE([disabling doxygen])
             _LW_DOXYGEN_DISABLED=1
             _LW_UPDATE_HAVE_DOXYGEN
        ])
    ])
    AC_ARG_ENABLE([doxygen-pdf],[AS_HELP_STRING([--enable-doxygen-pdf],[enable doxygen PDF output])],[
        AS_IF([test "x$enable_doxygen_pdf" != "xno"],[
            AS_IF([test "x$HAVE_DOXYGEN_PDF" = "xfalse"],[
                AC_MSG_FAILURE([Cannot enable doxygen-pdf due to missing dependencies.])
            ])
        ],[
             AC_MSG_NOTICE([disabling doxygen-pdf])
             _LW_DOXYGEN_PDF_DISABLED=1
             _LW_UPDATE_HAVE_DOXYGEN
        ])
    ])
])
