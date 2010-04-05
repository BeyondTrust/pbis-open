compat_subdirs = \
	auth \
	nsswitch

full_subdirs= \
	$(compat_subdirs) \
	gssntlm \
	lwmapsecurity \
	access

if ENABLE_COMPAT
SUBDIRS = $(compat_subdirs)
else
SUBDIRS = $(full_subdirs)
endif
