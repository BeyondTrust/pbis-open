noinst_PROGRAMS = tcon

tcon_SOURCES = tcon.c

tcon_LDFLAGS = -static

tcon_CPPFLAGS = 	      \
    -I@top_srcdir@  	      \
    -I@top_srcdir@/include    \
    -I@top_srcdir@/server/include

tcon_LDADD = \
	@top_builddir@/server/rdr/librdr.sys.la \
	@top_builddir@/smbkrb5/libsmbkrb5.la \
        @KRB5_LIBS@

