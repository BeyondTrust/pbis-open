AM_CPPFLAGS += -I$(top_srcdir)/include
AM_CFLAGS += -shared

lib_LTLIBRARIES = liblsapstore_plugin_test.la

liblsapstore_plugin_test_la_SOURCES = \
        lsapstore-plugin-test.c

liblsapstore_plugin_test_la_LIBADD = \
        $(LWADVAPI_NOTHR_LIBS) \
        $(LWBASE_NOTHR_LIBS)

liblsapstore_plugin_test_la_LDFLAGS = \
        -shared -module -avoid-version
