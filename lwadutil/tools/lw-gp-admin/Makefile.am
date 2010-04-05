bin_PROGRAMS = lw-gp-admin

lw_gp_admin_SOURCES = main.c \
		      regutils.c

lw_gp_admin_CPPFLAGS = \
		-I@top_srcdir@/api \
		-I@top_srcdir@/include

lw_gp_admin_LDADD = \
		@top_builddir@/api/liblwadutilapi.la \
		@LSASS_LIBS@ @GROUPPOLICY_LIBS@

lw_gp_admin_LDFLAGS = \
		@LSASS_LDFLAGS@ \
		@GROUPPOLICY_LDFLAGS@

