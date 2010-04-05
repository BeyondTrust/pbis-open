PROGRAM=LWEDSPlugIn
TYPE=dsplug

INFO=Info.plist

SOURCES=\
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
	LWISAuthAdapter.cpp \
	LWIBitVector.cpp \
	LWAuthAdapter.cpp \
	libgpldap/gpaquery.cpp \
	libgpldap/gpapolicyutils.cpp \
	libgpldap/gpadnshlp.cpp \
	libcentutils/ctstrutils.cpp \
	libcentutils/ctnetinfo.cpp \
	libcentutils/ctfileutils.cpp \
	libcentutils/cterr.cpp \
	LWIGroup.cpp \
	PlugInShell.cpp \
	LWIComputer.cpp \
	ADUtils.cpp \
	LWIUser.cpp \
	LWIRecTypeLookup.cpp

FRAMEWORKS=CoreFoundation

LOCAL_CFLAGS=-Ilibgpldap -Ilibcentutils -Wno-unknown-pragmas $(_CPPFLAGS) $(_CFLAGS)
LOCAL_LDFLAGS=$(_LDFLAGS) -bundle_loader /usr/sbin/DirectoryService

include Makefile.inc
