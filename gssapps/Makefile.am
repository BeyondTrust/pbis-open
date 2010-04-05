compat_subdirs = \
    gssclient \
    gssserver

full_subdirs= \
	$(compat_subdirs)

if ENABLE_COMPAT
SUBDIRS = $(compat_subdirs)
else
SUBDIRS = $(full_subdirs)
endif
