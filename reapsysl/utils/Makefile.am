
lib_LTLIBRARIES=librsysutils.la

librsysutils_la_SOURCES = \
	rsys-cfg.c \
	rsys-dlinkedlist.c \
	rsys-futils.c \
	rsys-mem.c \
	rsys-sysfuncs.c

noinst_HEADERS = \
    includes.h      \
	rsys-cfg_p.h   \
	rsys-sysfuncs_p.h

librsysutils_la_LDFLAGS =  \
    @LWREG_LIBS@\
    @LWADVAPI_LIBS@
