AM_CPPFLAGS =                   \
	@AM_CPPFLAGS@           \
	-I$(top_srcdir)/include

lib_LTLIBRARIES = liblwmapsecurity_lsass.la

liblwmapsecurity_lsass_la_SOURCES = \
	lwmapsecurity-lsass.c

liblwmapsecurity_lsass_la_CPPFLAGS =    \
	$(AM_CPPFLAGS)                  \
	$(LWBASE_INCLUDES)              \
	$(LWMAPSECURITY_INCLUDES)

liblwmapsecurity_lsass_la_LIBADD = \
	$(top_builddir)/client/liblsaclientthr.la \
	$(top_builddir)/client/rpc/liblsarpc.la \
	$(LWBASE_LIBS)

liblwmapsecurity_lsass_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-module -avoid-version -export-dynamic
