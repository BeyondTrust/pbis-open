AM_CFLAGS = -Wall
AM_CFLAGS += -I$(srcdir)/../include    \
             -I$(top_srcdir)/include   \
             -I$(top_srcdir)/libdomainjoin/include   \
             $(CENTUTILS_INCLUDE)       \
	     $(LWI_OS_FLAGS)

check_PROGRAMS = test_file_utils

test_file_utils_SOURCES = main.c

test_file_utils_LDADD = $(top_builddir)/libdomainjoin/libdomainjoin.la \
		       $(CENTUTILS_LDFLAGS) $(CENTUTILS_LIBS)
