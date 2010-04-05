ACLOCAL_AMFLAGS = -I m4

compat_subdirs = \
    utils        \
    include      \
    ipc          \
    client

full_subdirs=         \
    $(compat_subdirs) \
    logging_r         \
    parse             \
    server            \
    libedit           \
    shellutil         \
    test              \
    shell

if ENABLE_COMPAT
SUBDIRS = $(compat_subdirs)
else
SUBDIRS = $(full_subdirs)
endif

