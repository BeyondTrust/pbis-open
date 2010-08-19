plugindir = $(libdir)/krb5/plugins/libkrb5

plugin_LTLIBRARIES = liblwnet_service_locator.la

liblwnet_service_locator_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/include \
	$(KRB5_INCLUDES)

liblwnet_service_locator_la_LDFLAGS = \
	-module \
	$(KRB5_LDFLAGS)

liblwnet_service_locator_la_SOURCES = service_locator.c

liblwnet_service_locator_la_LIBADD = \
	$(top_builddir)/client/liblwnetclientapi.la \
	$(KRB5_LIBS) \
	$(LWADVAPI_LIBS)
