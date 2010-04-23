PROGRAM=LWIDSPlugIn
TYPE=dsplug

INFO=Info.plist

SOURCES=\
	LWAuthAdapter.cpp \
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
	LWISAuthAdapter.cpp \
	LWIUser.cpp \
	PlugInShell.cpp \
	ServerModule.c \
	ServerModuleLib.c \
	Utilities.cpp

FRAMEWORKS=CoreFoundation

LOCAL_CFLAGS=-Wno-unknown-pragmas $(_CPPFLAGS) $(_CFLAGS)
LOCAL_LDFLAGS=$(_LDFLAGS) -bundle_loader /usr/sbin/DirectoryService

include Makefile.inc
