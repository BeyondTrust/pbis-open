sbin_PROGRAMS = gpagentd

gpagentd_SOURCES = \
    globals.c         \
    gpaadinfo.c       \
    gpacse.c          \
    gpadomain.c       \
    gpaevents.c       \
    gpahandler.c      \
    gpalexer.c        \
    gpaparser.c       \
    gpapolicyutils.c  \
    gpaserver.c       \
    gpathread.c       \
    gpauser.c         \
    gpauserthread.c   \
    main.c

gpagentd_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/gpcse\
    -I@top_srcdir@/utils\
    @LWADVAPI_INCLUDES@

gpagentd_LDADD = \
    @top_builddir@/utils/libgputils.la       \
    @top_builddir@/server/ldap/libgpldap.la  \
    @top_builddir@/server/gpcse/libgpcses.la \
    @EVENTLOG_LIBS@ \
    @UUID_LIBS@     \
    @RT_LIBS@ \
    @DS_FRAMEWORK_LDFLAGS@\
    @LWREG_LIBS@\
    @LWADVAPI_LIBS@

