ACLOCAL_AMFLAGS = -I m4

compat_subdirs = \
       libadtool

full_subdirs= \
	include \
	$(compat_subdirs) \
    adtool

if ENABLE_COMPAT
SUBDIRS = $(compat_subdirs)
else
SUBDIRS = $(full_subdirs)
endif