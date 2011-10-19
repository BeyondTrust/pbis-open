AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include

noinst_PROGRAMS = test_stress_ad_provider

test_stress_ad_provider_SOURCES = \
	globals.c    \
	ladsconfig.c \
	main.c       \
	worker.c

test_stress_ad_provider_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-DLW_ENABLE_THREADS=1          \
	-I@top_srcdir@/server/include  \
	@NETLOGON_INCLUDES@

test_stress_ad_provider_LDADD = \
	@top_builddir@/common/liblsacommon.la     \
	@top_builddir@/logging_r/liblsalog_r.la \
	@NETLOGON_LIBS@ \
	@UUID_LIBS@     \
	@PTHREAD_LIBS@  \
	@DL_LIBS@