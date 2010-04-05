noinst_PROGRAMS = test_openldap

test_openldap_SOURCES = test_openldap.c

test_openldap_CPPFLAGS = \
	-DLW_ENABLE_THREADS=1         \
	-I@top_srcdir@/server/include \
	@OPENLDAP_INCLUDES@ \
	@KRB5_INCLUDES@

test_openldap_LDADD = \
	@top_builddir@/join/liblsajoin.la \
	@top_builddir@/server/api/liblsaserverapi.la \
	@top_builddir@/logging_r/liblsalog_r.la \
	@top_builddir@/common/liblsacommon.la \
	@OPENLDAP_LIBS@ \
	@KRB5_LIBS@ \
	$(RT_LIBS)

