ACLOCAL_AMFLAGS = -I m4

compat_subdirs =       \
       lwutils \
       api

full_subdirs= \
	include \
	$(compat_subdirs) \
        tools

if ENABLE_COMPAT
SUBDIRS = $(compat_subdirs)
else
SUBDIRS = $(full_subdirs)
endif

