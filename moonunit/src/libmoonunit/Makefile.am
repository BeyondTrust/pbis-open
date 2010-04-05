AUTOMAKE_OPTIONS= foreign

SUBDIRS=

lib_LTLIBRARIES=libmoonunit.la

libmoonunit_la_SOURCES = \
	error.c util.c test.c logger.c loader.c plugin.c option.c \
	interface.c type.c library.c resource.c

libmoonunit_la_LIBADD = @LIB_dl@ $(top_builddir)/src/libuipc/libuipc.la

AM_CPPFLAGS = \
	-I@top_srcdir@/include \
	-DPLUGIN_PATH="\"$(plugindir)\"" \
	-DPLUGIN_EXTENSION="\"$(moduleext)\""

if HAVE_MEMBEREXT
AM_CPPFLAGS += -DPLUGIN_MEMBER_EXTENSION="\"$(memberext)\""
endif

