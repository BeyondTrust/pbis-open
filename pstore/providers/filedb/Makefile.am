if ENABLE_STATIC_PROVIDERS
noinst_LTLIBRARIES = liblwps-filedb.la
PROVIDER_CFLAGS =
PROVIDER_LDFLAGS =
PROVIDER_LIBADD =
else
provider_LTLIBRARIES = liblwps-filedb.la
PROVIDER_CFLAGS = -shared
PROVIDER_LDFLAGS = -shared -module -avoid-version
PROVIDER_LIBADD = @top_builddir@/utils/liblwps-utils.la
endif

liblwps_filedb_la_SOURCES = \
	buffer_protocol.c  \
	db.c               \
	globals.c          \
	provider-main.c

liblwps_filedb_la_CFLAGS = \
	$(AM_CFLAGS) \
	@LIBUNISTR_INCLUDES@ \
	@LWMSG_INCLUDES@ \
	$(PROVIDER_CFLAGS)

liblwps_filedb_la_LIBADD = @LIBUNISTR_LIBS@ @LWMSG_LIBS@ $(PROVIDER_LIBADD) @PTHREAD_LIBS@

liblwps_filedb_la_LDFLAGS = $(PROVIDER_LDFLAGS) @LIBUNISTR_LDFLAGS@

noinst_HEADERS = \
	 buffer_protocol_p.h \
	 db_p.h \
	 externs.h \
	 includes.h \
	 provider-main_p.h
