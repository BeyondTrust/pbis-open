AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/include/public

noinst_LTLIBRARIES = liblwadvapi_lwunistr.la

liblwadvapi_lwunistr_la_SOURCES = lwunistr.c
liblwadvapi_lwunistr_la_LIBADD = $(LWBASE_NOTHR_LIBS)
liblwadvapi_lwunistr_la_CPPFLAGS = $(AM_CPPFLAGS) $(LWBASE_INCLUDES)
liblwadvapi_lwunistr_la_LDFLAGS = $(LWBASE_LDFLAGS)
