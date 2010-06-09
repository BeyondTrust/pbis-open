## Process this file with automake to produce Makefile.in
include $(top_srcdir)/include.mk

noinst_PROGRAMS = rpcping

rpcping_SOURCES = main.c

rpcping_LDADD = $(top_builddir)/dcelib/libdcerpc.la $(top_builddir)/uuid/libuuid.la
