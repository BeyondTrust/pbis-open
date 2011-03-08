AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include

bin_PROGRAMS = lw-privilege

lw_privilege_SOURCES = \
        common.c \
        privilege.c \
        system_access_right.c \
        security.c \
        account.c

lw_privilege_CPPFLAGS = \
        $(AM_CPPFLAGS)

lw_privilege_LDADD = \
	$(top_builddir)/client/rpc/liblsarpc.la \
	-llwbase_nothr \
	-llwadvapi_nothr
