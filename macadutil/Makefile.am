ACLOCAL_AMFLAGS = -I m4

compat_subdirs =       \
       lwutils \
       api

full_subdirs= \
	include \
	$(compat_subdirs) \
        tests

if ENABLE_COMPAT
SUBDIRS = $(compat_subdirs)
else
SUBDIRS = $(full_subdirs)
endif

