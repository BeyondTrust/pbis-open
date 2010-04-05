ACLOCAL_AMFLAGS = -I m4

compat_subdirs = \
	    utils    \
	    client   \
	    dlclient

full_subdirs= \
	$(compat_subdirs) \
	etc               \
	include           \
        scripts           \
	server            \
	tools

if ENABLE_COMPAT
SUBDIRS = $(compat_subdirs)
else
SUBDIRS = $(full_subdirs)
endif
