AM_CPPFLAGS = -I$(top_srcdir)/include

lib_LTLIBRARIES = libktkrb5.la

libktkrb5_la_CFLAGS = \
       $(AM_CFLAGS) \
       @KRB5_INCLUDES@      \
       @OPENLDAP_INCLUDES@

libktkrb5_la_SOURCES = \
       globals.c    \
       krbtgt.c     \
       ktkrb5.c     \
       keytab.c     \
       keytab_w16.c \
       ldap.c       \
       ldap_w16.c   \
       ktmem.c      \
       ktstr.c

libktkrb5_la_LDFLAGS = \
       @KRB5_LDFLAGS@      \
       @OPENLDAP_LDFLAGS@

libktkrb5_la_LIBADD = \
       -llwbase                           \
       @OPENLDAP_LIBS@                    \
       @KRB5_LIBS@                        \
       @PTHREAD_LIBS@

