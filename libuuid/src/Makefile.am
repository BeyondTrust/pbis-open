lib_LTLIBRARIES=libuuid.la

AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/include/uuid

libuuid_la_SOURCES = \
	clear.c \
	copy.c \
	pack.c \
	unparse.c \
	compare.c \
	gen_uuid.c \
	isnull.c \
	parse.c \
	unpack.c \
	uuid_time.c

libuuid_la_LDFLAGS = $(AM_LDFLAGS) -version-number 1:2:1