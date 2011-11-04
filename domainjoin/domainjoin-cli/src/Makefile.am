AM_CFLAGS = -Wall
AM_CFLAGS += -I$(srcdir)/../include    \
             -I$(top_srcdir)/include   \
             -I$(top_srcdir)/libdomainjoin/include   \
             $(CENTUTILS_INCLUDE)       \
	     $(LWI_OS_FLAGS)

bin_PROGRAMS = domainjoin-cli

domainjoin_cli_SOURCES = djcli.c \
       main.c

domainjoin_cli_LDADD = $(top_builddir)/libdomainjoin/libdomainjoin.la \
		       $(CENTUTILS_LDFLAGS) \
                       $(CENTUTILS_LIBS)    \
                       $(LSASS_LIBS) \
                       -llwadvapi_nothr
