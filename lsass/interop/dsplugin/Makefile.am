bundledir=$(libdir)/LWDSPlugin.dsplug
contentsdir=$(bundledir)/Contents
macosdir = $(contentsdir)/MacOS

SUBDIRS = etc

macos_LTLIBRARIES = LWDSPlugIn.la
contents_DATA = Info.plist

AM_CFLAGS=@AM_CFLAGS@ \
	-Wno-unknown-pragmas \
	-pipe -fno-common -fpascal-strings -Wno-four-char-constants -Wall -Werror -mmacosx-version-min=10.4

INFO=Info.plist

LWDSPlugIn_la_SOURCES= \
	ServerModuleLib.c \
	ServerModule.c \
	LWIDirNodeQuery.cpp \
	LWIComputerList.cpp \
	LWIRecordQuery.cpp \
	LWIQuery.cpp \
	LWIAttrValDataQuery.cpp \
	LWIRecordListQuery.cpp \
	Utilities.cpp \
	LWIAttrLookup.cpp \
	LWICRC.cpp \
	LWIComputerGroup.cpp \
	LWIBitVector.cpp \
	LWIGroup.cpp \
	PlugInShell.cpp \
	LWIComputer.cpp \
	LWIUser.cpp \
	LWIRecTypeLookup.cpp \
	libgpldap/gpapolicyutils.c \
	libgpldap/gpadnshlp.c \
        macadutil/api.c \
        macadutil/adinfo.c \
        macadutil/aduserinfo.c \
        macadutil/adukrb5.c \
        macadutil/aducopy.c \
        macadutil/cfgparser.c \
        macadutil/credcontext.c \
        macadutil/directory.c \
        macadutil/filexfer.c \
        macadutil/gpcache.c \
        macadutil/gss.c \
        macadutil/macerror.c \
        macadutil/mcxutil.c \
        macadutil/notify.c \
        macadutil/netinfo.c \
        macadutil/policyutils.c \
        macadutil/xfer.c

LWDSPlugIn_la_CFLAGS=$(AM_CFLAGS) -I$(top_srcdir)/interop/auth/include

LWDSPlugIn_la_CXXFLAGS=$(AM_CFLAGS)\
	-I$(top_srcdir)/interop/auth/include \
	-fvisibility-inlines-hidden

LWDSPlugIn_la_CPPFLAGS=@AM_CPPFLAGS@

LWDSPlugIn_la_LDFLAGS= \
     -module -shrext '' -avoid-version \
     -bundle_loader /usr/sbin/DirectoryService \
     -framework CoreFoundation

LWDSPlugIn_la_LIBADD =  \
    $(top_builddir)/common/liblsacommon.la \
    $(top_builddir)/interop/auth/common/liblsaauth.la \
    $(top_builddir)/client/liblsaclient.la \
     -llwnetclientapi \
     -llwioclient \
     -llwadvapi \
     -lldap \
     -lkrb5

Info.plist: Info.plist.in
	$(srcdir)/process-info-plist.pl -v $(MAIN_VERSION) -o "$@" $(srcdir)/Info.plist.in
