noinst_LTLIBRARIES = libutility.la

libutility_la_SOURCES = \
							csv.c \
							data.c \
							ldif.c \
							group.c \
							user.c \
							utility.c \
							log.c 

libutility_la_CPPFLAGS = \
							-I$(top_srcdir)/include \
							$(AM_CPPFLAGS)

libutility_la_CFLAGS = \
							$(AM_CFLAGS)

libutility_la_LIBADD =
libutility_la_LDFLAGS = \
							-g $(AM_LDFLAGS)
