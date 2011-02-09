bundledir=$(libdir)/SecurityAgentPlugins/Likewise.bundle
contentsdir=$(bundledir)/Contents
macosdir = $(contentsdir)/MacOS

macos_LTLIBRARIES = Likewise.la
contents_DATA = Info.plist

AM_CFLAGS=@AM_CFLAGS@ \
	-Wno-unknown-pragmas \
	-pipe -fno-common -fpascal-strings -Wno-four-char-constants \
	-Wall -Werror -mmacosx-version-min=10.5

INFO=Info.plist

Likewise_la_SOURCES = \
	AuthPlugin.c

Likewise_la_CFLAGS = $(AM_CFLAGS)

Likewise_la_CPPFLAGS = @AM_CPPFLAGS@

Likewise_la_LDFLAGS = \
     -module -shrext '' -avoid-version \
     -framework CoreFoundation \
     -framework Kerberos

Likewise_la_LIBADD = @top_builddir@/common/liblsacommon.la

Info.plist: Info.plist.in
	$(srcdir)/../dsplugin/process-info-plist.pl -v $(MAIN_VERSION) \
	    -o "$@" $(srcdir)/Info.plist.in
