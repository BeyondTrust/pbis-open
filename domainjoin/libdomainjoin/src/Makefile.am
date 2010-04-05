ACLOCAL_AMFLAGS = -I m4

if ENABLE_MINIMAL
ADDITIONAL_CFLAGS = "-DMINIMAL_JOIN=1"
ADDITIONAL_LIBS =
else
ADDITIONAL_CFLAGS =
ADDITIONAL_LIBS = \
	$(LWDNS_LIBS)   \
	@EVENTLOG_LDFLAGS@ \
	@EVENTLOG_LIBS@\
	@LWREG_LDFLAGS@\
	@LWREG_LIBS@
endif

AM_CFLAGS = $(CPPFLAGS) -Wall -Werror-implicit-function-declaration

AM_CFLAGS += -I$(top_srcdir)/include  \
          -I$(srcdir)/../include \
          @CENTUTILS_INCLUDE@    \
          @LIBXML2_INCLUDE@      \
          @NETLOGON_INCLUDE@     \
          @LWDNS_INCLUDE@        \
          @EVENTLOG_INCLUDES@     \
          @LWREG_INCLUDES@     \
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
        @LIB_XNET@

libdomainjoin_la_SOURCES = \
       djparsehosts.c \
       djhostinfo.c   \
       djauthinfo.c   \
       djkrb5conf.c   \
       djauditing.c   \
       djlogger.c     \
       djdistroinfo.c \
       djstr.c    \
       djroutines.c   \
       djprocutils.c  \
       djmodule.c \
       djapi.c

if ENABLE_MINIMAL
else
libdomainjoin_la_SOURCES += \
       djsystemscripts.c \
       aix/djsecuser.c    \
       djnsswitch.c    \
       djpamconf.c    \
       aix/djlogincfg.c   \
       aix/djmethodcfg.c  \
       djdaemonmgr.c \
       aix/djaixparser.c \
       mac/djconfig_mac.c \
       djsshconf.c    \
       djddns.c

if BUILD_MACCODE
EXTRA_libdomainjoin_la_SOURCES = djdaemonmgr_nonmac.c
libdomainjoin_la_SOURCES += mac/djdaemonmgr_mac.c
else
EXTRA_libdomainjoin_la_SOURCES = mac/djdaemonmgr_mac.c
libdomainjoin_la_SOURCES += djdaemonmgr_nonmac.c
endif

endif
