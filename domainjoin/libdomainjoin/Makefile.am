ACLOCAL_AMFLAGS = -I m4

if ENABLE_MINIMAL
ADDITIONAL_CFLAGS = "-DMINIMAL_JOIN=1"
ADDITIONAL_LIBS =
else
ADDITIONAL_CFLAGS =
ADDITIONAL_LIBS = \
	$(LWDNS_LIBS)   \
	$(EVENTLOG_LDFLAGS) \
	$(EVENTLOG_LIBS)\
	$(LWREG_LDFLAGS)\
	$(LWREG_LIBS)
endif

AM_CFLAGS = $(CPPFLAGS) -Wall -Werror-implicit-function-declaration

AM_CFLAGS += -I$(top_srcdir)/include  \
          -I$(srcdir)/include \
          $(CENTUTILS_INCLUDE)    \
          $(LIBXML2_INCLUDE)      \
          $(NETLOGON_INCLUDE)     \
          $(LWDNS_INCLUDE)        \
          $(EVENTLOG_INCLUDES)     \
          $(LWREG_INCLUDES)     \
          $(LSASS_INCLUDES)     \
          $(LWI_OS_FLAGS)        \
          $(ADDITIONAL_CFLAGS)

SCRIPTDIR = $(bindir)
AM_CPPFLAGS = -DSCRIPTDIR=\"$(SCRIPTDIR)\" -DLIBDIR=\"$(libdir)\" -DSYSCONFDIR=\"$(sysconfdir)\" -DDYNLIBEXT=\"$(DYNLIBEXT)\" $(LWI_OS_FLAGS)

lib_LTLIBRARIES = libdomainjoin.la

libdomainjoin_la_LIBADD = \
	$(CENTUTILS_LDFLAGS) \
	$(CENTUTILS_LIBS) \
	$(NETLOGON_LDFLAGS) \
	$(NETLOGON_LIBS) \
	$(LIBXML2_LDFLAGS) \
	$(LIBXML2_LIBS) \
	$(ADDITIONAL_LIBS) \
	$(LSASS_LDFLAGS) \
	$(LSASS_LIBS) \
        -llwsm \
    $(LWADVAPI_LDFLAGS) \
    $(LWADVAPI_LIBS) \
        $(LIB_XNET)

include_HEADERS = \
	include/djdistroinfo.h

libdomainjoin_la_SOURCES = \
       src/djparsehosts.c \
       src/djhostinfo.c   \
       src/djauthinfo.c   \
       src/djkrb5conf.c   \
       src/djauditing.c   \
       src/djlogger.c     \
       src/djdistroinfo.c \
       src/djstr.c    \
       src/djregistry.c \
       src/djroutines.c   \
       src/djprocutils.c  \
       src/djmodule.c \
       src/djapi.c

if ENABLE_MINIMAL
else
libdomainjoin_la_SOURCES += \
       src/djsystemscripts.c \
       src/aix/djsecuser.c    \
       src/djnsswitch.c    \
       src/djpamconf.c    \
       src/aix/djlogincfg.c   \
       src/aix/djmethodcfg.c  \
       src/djdaemonmgr.c \
       src/aix/djaixparser.c \
       src/mac/djconfig_mac.c \
       src/djsshconf.c    \
       src/djddns.c \
       src/djservicemgr.c

if BUILD_MACCODE
EXTRA_libdomainjoin_la_SOURCES = src/djdaemonmgr_nonmac.c
libdomainjoin_la_SOURCES += src/mac/djdaemonmgr_mac.c
else
EXTRA_libdomainjoin_la_SOURCES = src/mac/djdaemonmgr_mac.c
libdomainjoin_la_SOURCES += src/djdaemonmgr_nonmac.c
endif

endif
