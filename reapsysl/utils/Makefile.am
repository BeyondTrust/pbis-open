
lib_LTLIBRARIES=librsysutils.la

librsysutils_la_SOURCES = \
	rsys-cfg.c \
	rsys-dlinkedlist.c \
	rsys-error.c \
	rsys-futils.c \
	rsys-mem.c \
	rsys-paths.c \
	rsys-stack.c \
	rsys-str.c \
	rsys-strerror.c \
	rsys-sysfuncs.c

noinst_HEADERS = \
    includes.h      \
	rsys-cfg_p.h   \
	rsys-sysfuncs_p.h   \
	rsys-error_p.h

librsysutils_la_LDFLAGS =  \
    @top_builddir@/logging/librsyslogging.la\
    @LWREG_LIBS@\
    @LWADVAPI_LIBS@
