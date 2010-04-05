AM_CPPFLAGS = -I$(top_srcdir)/include

lib_LTLIBRARIES = libschannel.la

libschannel_la_CFLAGS = \
       @CRYPTO_INCLUDES@

libschannel_la_SOURCES = \
       schn_util.c \
       schn_init.c \
       schn_wrap.c \
       schn_unwrap.c

libschannel_la_LDFLAGS = \
       @CRYPTO_LDFLAGS@

libschannel_la_LIBADD = \
       @CRYPTO_LIBS@
