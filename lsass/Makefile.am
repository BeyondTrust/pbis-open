ACLOCAL_AMFLAGS = -I m4

compat_subdirs = \
	common \
    client \
	interop

full_subdirs= \
	$(compat_subdirs) \
	logging_r \
	lsaunistr \
    join \
    server \
    tools \
    etc \
    include \
    scripts \
    tests \
    docs

if ENABLE_COMPAT
SUBDIRS = $(compat_subdirs)
else
SUBDIRS = $(full_subdirs)
endif
