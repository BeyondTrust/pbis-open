compat_subdirs = \
            include \
	    utils   \
	    ktkrb5  \
	    test

full_subdirs= \
	$(compat_subdirs)

if ENABLE_COMPAT
SUBDIRS = $(compat_subdirs)
else
SUBDIRS = $(full_subdirs)
endif
