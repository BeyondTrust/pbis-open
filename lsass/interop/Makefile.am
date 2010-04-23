compat_subdirs = \
	auth \
	nsswitch

if PLATFORM_MAC
mac_full_subdirs= \
        lwidsplugin
else
mac_full_subdirs=
endif

full_subdirs= \
	$(compat_subdirs) \
	gssntlm \
	lwmapsecurity \
	access

if ENABLE_COMPAT
SUBDIRS = $(compat_subdirs)
else
SUBDIRS = $(full_subdirs) $(mac_full_subdirs)
endif
