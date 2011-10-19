if ENABLE_STATIC_PROVIDERS
noinst_LTLIBRARIES = liblwps-regdb.la
PROVIDER_CFLAGS =
PROVIDER_LDFLAGS =
PROVIDER_LIBADD =
else
provider_LTLIBRARIES = liblwps-regdb.la
PROVIDER_CFLAGS = -shared
PROVIDER_LDFLAGS = -shared -module -avoid-version
PROVIDER_LIBADD = @LWREG_LDFLAGS@ @top_builddir@/utils/liblwps-utils.la
endif

AM_CPPFLAGS = -I$(top_srcdir)/include 

liblwps_regdb_la_CPPFLAGS = \
        $(AM_CPPFLAGS) \
        @LWREG_INCLUDES@

liblwps_regdb_la_SOURCES = \
	globals.c \
	provider-main.c \
	pstoresec.c

liblwps_regdb_la_CFLAGS = \
	$(AM_CFLAGS) \
	@LWMSG_INCLUDES@ \
	$(PROVIDER_CFLAGS)

liblwps_regdb_la_LIBADD = @LWMSG_LIBS@ \
                          @LWREG_LIBS@ \
                          $(PROVIDER_LIBADD) \
                          @PTHREAD_LIBS@

liblwps_regdb_la_LDFLAGS = $(AM_LDFLAGS) \
                           $(PROVIDER_LDFLAGS)


noinst_HEADERS = \
	 externs.h \
	 includes.h \
	 provider-main_p.h
