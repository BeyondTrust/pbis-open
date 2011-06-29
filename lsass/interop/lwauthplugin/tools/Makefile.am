libexec_PROGRAMS = authmechanism

AM_CFLAGS=@AM_CFLAGS@ \
	-Wno-unknown-pragmas \
	-pipe -fno-common -fpascal-strings -Wno-four-char-constants \
	-Wall -Werror -mmacosx-version-min=10.5

authmechanism_SOURCES = \
	authmechanism.c

authmechanism_CFLAGS = $(AM_CFLAGS)

authmechanism_CPPFLAGS = @AM_CPPFLAGS@

authmechanism_LDADD = \
	-ldl
