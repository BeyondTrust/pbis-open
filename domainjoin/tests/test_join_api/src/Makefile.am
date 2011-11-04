AM_CFLAGS = -Wall
AM_CFLAGS += -I$(srcdir)/../include    \
             -I$(top_srcdir)/include   \
             -I$(top_srcdir)/libdomainjoin/include   \
             $(CENTUTILS_INCLUDE)       \
	         $(LWI_OS_FLAGS)

check_PROGRAMS = test_join_api

test_join_api_SOURCES = main.c

test_join_api_LDADD = $(top_builddir)/libdomainjoin/libdomainjoin.la \
		       $(CENTUTILS_LDFLAGS) $(CENTUTILS_LIBS)
