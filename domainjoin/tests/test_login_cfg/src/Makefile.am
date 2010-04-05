AM_CFLAGS = -Wall
AM_CFLAGS += -I$(srcdir)/../include    \
             -I$(top_srcdir)/include   \
             -I$(top_srcdir)/libdomainjoin/include   \
             $(CENTUTILS_INCLUDE)       \
	     $(LWI_OS_FLAGS)

check_PROGRAMS = test_login_cfg

test_login_cfg_SOURCES = main.c

test_login_cfg_LDADD = $(top_builddir)/libdomainjoin/src/libdomainjoin.la \
		       $(CENTUTILS_LDFLAGS) $(CENTUTILS_LIBS)
