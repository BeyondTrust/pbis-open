ACLOCAL_AMFLAGS = -I m4

## Process this file with automake to produce Makefile.in
include $(top_srcdir)/include.mk

SUBDIRS=flex-2.5.4 libdcethread idl_compiler include idl_lib uuid ncklib dcelib rpcd demos . perf

EXTRA_DIST=include.mk buildconf

dist-hook:
	-chmod -R a+r $(distdir)
	$(TAR) choIf $(distdir).tar.bz2 $(distdir)

