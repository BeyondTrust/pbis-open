## Process this file with automake to produce Makefile.in
include $(top_srcdir)/include.mk

bin_PROGRAMS = rpcping

rpcping_SOURCES = main.c

rpcping_LDADD = $(top_builddir)/dcelib/libdcerpc.la $(top_builddir)/uuid/libuuid.la

rpcping_CFLAGS = -Wall -Werror
