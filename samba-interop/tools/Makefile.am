AM_CPPFLAGS += -I$(top_srcdir)/include -I$(top_srcdir)/libtdb/include -I$(top_srcdir)/pstore-plugin

bin_PROGRAMS = samba-interop-install

samba_interop_install_SOURCES = \
	samba-interop-install.c

samba_interop_install_LDADD = -llsaclient $(top_builddir)/pstore-plugin/libsamba-pstore-plugin.la -llwadvapi -llwbase_nothr
