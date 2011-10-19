noinst_PROGRAMS = transactnp

transactnp_SOURCES = transactnp.c

transactnp_LDFLAGS = -static

transactnp_CPPFLAGS = 	      \
    -I@top_srcdir@  	      \
    -I@top_srcdir@/include    \
    -I@top_srcdir@/server/include

transactnp_LDADD = \
	@top_builddir@/server/rdr/librdr.sys.la \
	@top_builddir@/server/smbtransport/libsmbtransport.la
