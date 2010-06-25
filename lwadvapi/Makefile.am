lib_LTLIBRARIES = liblwadvapi.la

SUBDIRS = include unthreaded

liblwadvapi_la_LIBADD = \
	unthreaded/liblwadvapi_nothr.la

if !ENABLE_COMPAT
SUBDIRS += threaded
liblwadvapi_la_LIBADD += \
	threaded/liblwadvapi_threaded.la
endif

liblwadvapi_la_SOURCES = dummy.c
