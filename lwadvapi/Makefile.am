lib_LTLIBRARIES = liblwadvapi.la

SUBDIRS = include

liblwadvapi_la_LIBADD = \
	unthreaded/liblwadvapi_nothr.la

if !ENABLE_COMPAT
SUBDIRS += join
liblwadvapi_la_LIBADD += \
	join/liblwadvapi_join.la
endif

SUBDIRS += unthreaded

liblwadvapi_la_SOURCES = dummy.c
