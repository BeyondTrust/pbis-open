AM_CPPFLAGS =                   \
	@AM_CPPFLAGS@           \
	-I$(top_srcdir)/include

lib_LTLIBRARIES = liblwmapsecurity_lsass.la

noinst_LTLIBRARIES = liblwmapsecurity_internal_lsass.la

liblwmapsecurity_lsass_la_SOURCES = \
	lwmapsecurity-lsass.c

liblwmapsecurity_lsass_la_CPPFLAGS =    \
	$(AM_CPPFLAGS)                  \
	$(LWBASE_INCLUDES)              \
	$(LWMAPSECURITY_INCLUDES)

liblwmapsecurity_lsass_la_LIBADD = \
	$(top_builddir)/client/liblsaclient.la \
	$(top_builddir)/client/rpc/liblsarpc.la \
	$(LWBASE_LIBS)

liblwmapsecurity_lsass_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-module -avoid-version -export-dynamic

liblwmapsecurity_internal_lsass_la_SOURCES = \
	lwmapsecurity-lsass.c                \
	lsass-calls.c

liblwmapsecurity_internal_lsass_la_CPPFLAGS = \
	$(AM_CPPFLAGS)                        \
        -DLSASS_INTERNAL_PLUGIN               \
	$(LWBASE_INCLUDES)                    \
	-I$(top_srcdir)/server/include

liblwmapsecurity_internal_lsass_la_LIBADD = \
	$(LWBASE_LIBS)

liblwmapsecurity_internal_lsass_la_LDFLAGS = \
	$(AM_LDFLAGS)
