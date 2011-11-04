AM_CPPFLAGS = -I$(top_srcdir)/include

noinst_PROGRAMS = lw-addevent

lw_addevent_SOURCES = main.c
lw_addevent_LDADD = \
    @top_builddir@/client/libeventlog.la \
    @top_builddir@/utils/libeventlogutils.la \
    -ldcerpc

