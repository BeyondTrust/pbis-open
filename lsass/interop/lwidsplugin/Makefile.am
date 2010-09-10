bundledir=$(libdir)/LWIDSPlugin.dsplug
contentsdir=$(bundledir)/Contents
macosdir = $(contentsdir)/MacOS

macos_LTLIBRARIES = LWIDSPlugIn.la
contents_DATA = Info.plist

AM_CFLAGS=@AM_CFLAGS@ \
	-Wno-unknown-pragmas \
	-pipe -fno-common -fpascal-strings -Wno-four-char-constants -Wall -Werror -mmacosx-version-min=10.4

INFO=Info.plist

LWIDSPlugIn_la_SOURCES=\
	LWIAttrLookup.cpp \
	LWIAttrValDataQuery.cpp \
	LWIBitVector.cpp \
	LWICRC.cpp \
	LWIDirNodeQuery.cpp \
	LWIGroup.cpp \
	LWIQuery.cpp \
	LWIRecTypeLookup.cpp \
	LWIRecordListQuery.cpp \
	LWIRecordQuery.cpp \
	LWIUser.cpp \
	PlugInShell.cpp \
	ServerModule.c \
	ServerModuleLib.c \
	Utilities.cpp \
        macadutil/api.c \
        macadutil/aduserinfo.c \
        macadutil/adukrb5.c \
        macadutil/cfgparser.c \
        macadutil/credcontext.c \
        macadutil/directory.c \
        macadutil/gss.c \
        macadutil/macerror.c

LWIDSPlugIn_la_CFLAGS=$(AM_CFLAGS)

LWIDSPlugIn_la_CXXFLAGS=$(AM_CFLAGS)\
	-fvisibility-inlines-hidden

LWIDSPlugIn_la_CPPFLAGS=@AM_CPPFLAGS@

LWIDSPlugIn_la_LDFLAGS=\
     -module -shrext '' -avoid-version -bundle_loader \
     /usr/sbin/DirectoryService \
     -framework CoreFoundation \
     -llwnetclientapi \
     -llwioclient \
     -llwadvapi \
     -lldap \
     -lkrb5

LWIDSPlugIn_la_LIBADD =  \
    $(top_builddir)/common/liblsacommon.la \
    $(top_builddir)/client/liblsaclient.la

Info.plist: Info.plist.in
	$(srcdir)/process-info-plist.pl -v $(MAIN_VERSION) -o "$@" $(srcdir)/Info.plist.in
