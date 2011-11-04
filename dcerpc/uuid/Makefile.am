## Process this file with automake to produce Makefile.in
# Makefile for the UUID library and tools

include $(top_srcdir)/include.mk

AM_CPPFLAGS += -D_POSIX_C_SOURCE

noinst_LTLIBRARIES = libuuid.la

libuuid_la_SOURCES = uuid.c uuidsys.c get_802_addr.c

noinst_HEADERS= uuid_i.h

# Build the UUID generation tool
bin_PROGRAMS = uuid
uuid_SOURCES = uuidtool.c
uuid_LDADD = ./libuuid.la $(LIB_socket)

