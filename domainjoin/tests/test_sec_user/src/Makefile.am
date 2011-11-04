AM_CFLAGS = -Wall
AM_CFLAGS += -I$(srcdir)/../include    \
             -I$(top_srcdir)/include   \
             -I$(top_srcdir)/libdomainjoin/include   \
             $(CENTUTILS_INCLUDE)       \
	     $(LWI_OS_FLAGS)

check_PROGRAMS = test_sec_user

test_sec_user_SOURCES = main.c

test_sec_user_LDADD = $(top_builddir)/libdomainjoin/libdomainjoin.la \
		       $(CENTUTILS_LDFLAGS) $(CENTUTILS_LIBS)
