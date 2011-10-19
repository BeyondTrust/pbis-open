AM_CPPFLAGS = -I$(top_srcdir)/include

if ENABLE_STATIC_PROVIDERS
noinst_LTLIBRARIES = liblwps-sqldb.la
PROVIDER_CFLAGS =
PROVIDER_LDFLAGS =
PROVIDER_LIBADD =
else
provider_LTLIBRARIES = liblwps-sqldb.la
PROVIDER_CFLAGS = -shared
PROVIDER_LDFLAGS = -shared -module -avoid-version
PROVIDER_LIBADD = @top_builddir@/utils/liblwps-utils.la
endif

liblwps_sqldb_la_SOURCES = \
	db.c               \
	globals.c          \
	provider-main.c

liblwps_sqldb_la_CFLAGS = \
	@SQLITE_INCLUDES@ \
	$(PROVIDER_CFLAGS)

liblwps_sqldb_la_LIBADD = @SQLITE_LIBS@ $(PROVIDER_LIBADD)

liblwps_sqldb_la_LDFLAGS = $(PROVIDER_LDFLAGS) @SQLITE_LDFLAGS@

noinst_HEADERS = \
	 db_p.h \
	 dbquery.h \
	 externs.h \
	 provider-main_p.h
