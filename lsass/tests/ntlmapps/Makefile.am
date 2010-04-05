compat_subdirs = \
    ntlmclient \
    ntlmserver

full_subdirs= \
	$(compat_subdirs)

if ENABLE_COMPAT
SUBDIRS = $(compat_subdirs)
else
SUBDIRS = $(full_subdirs)
endif
