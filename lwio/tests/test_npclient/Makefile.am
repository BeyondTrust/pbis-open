AM_CPPFLAGS = -I$(top_srcdir)/include

noinst_PROGRAMS = test_npclient

test_npclient_SOURCES = main.c

test_npclient_CPPFLAGS = $(AM_CPPFLAGS) -DUNICODE

test_npclient_LDFLAGS = -static

test_npclient_LDADD = @top_builddir@/client/liblwioclient.la
