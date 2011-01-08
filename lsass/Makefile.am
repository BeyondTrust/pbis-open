ACLOCAL_AMFLAGS = -I m4

compat_subdirs = \
    common \
    client \
    interop

full_subdirs= \
    $(compat_subdirs) \
    pstore \
    logging_r \
    join \
    server \
    tools \
    etc \
    include \
    scripts \
    docs 

if ENABLE_COMPAT
SUBDIRS = $(compat_subdirs)
else
SUBDIRS = $(full_subdirs)
endif
