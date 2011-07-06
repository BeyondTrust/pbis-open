lib_LTLIBRARIES = libauthplugin.la

AM_CFLAGS=@AM_CFLAGS@ \
	-Wno-unknown-pragmas \
	-pipe -fno-common -fpascal-strings -Wno-four-char-constants \
	-Wall -Werror -mmacosx-version-min=10.5

libauthplugin_la_SOURCES = \
	mechanism.c

libauthplugin_la_CFLAGS = $(AM_CFLAGS)
libauthplugin_la_CPPFLAGS = @AM_CPPFLAGS@
libauthplugin_la_LDFLAGS = \
     -framework CoreFoundation
