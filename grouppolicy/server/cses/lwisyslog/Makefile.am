gpextnlibdir = @EXTNLIBDIR@
gpextnlib_LTLIBRARIES = liblwisyslog.la

liblwisyslog_la_SOURCES = \
	  lwisyslogpolicy.c\
      lwisyslog.c\
      lwisyslogng.c\
      lwisyslogutils.c\
	  lwisyslogwrite.c

liblwisyslog_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I@top_srcdir@/server/include\
    -I@top_srcdir@/server/gpcse\
    -I@top_srcdir@/utils/\
    @LWADVAPI_INCLUDES@

liblwisyslog_la_LIBADD = \
    @top_builddir@/utils/libgputils.la\
    @LWADVAPI_LIBS@

liblwisyslog_la_LDFLAGS = \
    -module -export-dynamic

