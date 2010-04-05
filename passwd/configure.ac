#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([passwd replacement], 1.0, support@centeris.com)
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([main.c])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
gl_EARLY

# Checks for libraries.
AC_SEARCH_LIBS([pam_start], [pam])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h termios.h security/pam_appl.h usersec.h])

gl_INIT

AC_LANG_WERROR

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_TYPE_UID_T
AC_CHECK_TYPES([struct pam_conv], [], [AC_MSG_ERROR([Missing pam_conv])],
[#ifdef HAVE_SECURITY_PAM_APPL_H
#include <security/pam_appl.h>
#endif
])
AC_CHECK_TYPES([struct pam_repository], [], [],
[#ifdef HAVE_SECURITY_PAM_APPL_H
#include <security/pam_appl.h>
#endif
])

AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
    [#ifdef HAVE_SECURITY_PAM_APPL_H
    #include <security/pam_appl.h>
    #endif
    ],
    [
        struct pam_conv conversation = { (int (*)(
            int, const struct pam_message **, const struct pam_response **, void *))NULL,
            NULL
        };
    ])],
    [
    CONST_PAM_MESSAGE="const struct pam_message"
    ], [
    CONST_PAM_MESSAGE="struct pam_message"
    ]
)
AC_DEFINE_UNQUOTED(
    [CONST_PAM_MESSAGE], $CONST_PAM_MESSAGE,
    [This is the type of 'const struct pam_message' in system parameters]
)

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset strdup chpass])

AC_CONFIG_FILES([Makefile lib/Makefile])
AC_OUTPUT
