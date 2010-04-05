AM_CPPFLAGS = -I$(top_srcdir)/include

noinst_PROGRAMS = test_npclient2

test_npclient2_SOURCES = main.c

test_npclient2_CPPFLAGS = $(AM_CPPFLAGS) -DUNICODE

test_npclient2_LDFLAGS = -static

test_npclient2_LDADD = @top_builddir@/client/liblwioclient.la
