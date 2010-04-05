if USE_INSTALLED_GLIB
included_glib_includes = @GLIB_CFLAGS@
pkg_config_LDADD=@GLIB_LIBS@
else
GLIB_SUBDIR = glib-1.2.10
included_glib_includes = -I./glib-1.2.10
pkg_config_LDADD=glib-1.2.10/libglib.la
endif
SUBDIRS = $(GLIB_SUBDIR) check

m4dir = $(datadir)/aclocal
m4_DATA = pkg.m4

man_MANS = pkg-config.1
EXTRA_DIST = $(m4_DATA) $(man_MANS) README.win32

bin_PROGRAMS = pkg-config
AM_CFLAGS=@WARN_CFLAGS@

INCLUDES=-DPKG_CONFIG_PC_PATH="\"$(pc_path)\"" $(included_glib_includes)

pkg_config_SOURCES= \
	pkg.h \
	pkg.c \
	partial-glib.h \
	partial-glib.c \
	parse.h \
	parse.c \
	main.c \
	findme.c \
	findme.h \
	popt.c \
	popt.h \
	poptconfig.c \
	popthelp.c \
	poptint.h \
	poptparse.c

