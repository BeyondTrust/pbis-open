AM_CPPFLAGS = -I$(top_srcdir)/include

noinst_LTLIBRARIES = libktkrb5.la

libktkrb5_la_CFLAGS =          \
       $(AM_CFLAGS)            \
       $(KRB5_INCLUDES)        \
       $(OPENLDAP_INCLUDES)    \
       $(LWADVAPI_INCLUDES)    \
       $(top_builddir)/include

libktkrb5_la_SOURCES = \
       keytab.c        \
       ldap.c

libktkrb5_la_LDFLAGS =      \
       $(KRB5_LDFLAGS)      \
       $(OPENLDAP_LDFLAGS)  \
       $(LWADVAPI_LDFLAGS)

libktkrb5_la_LIBADD =       \
       $(LWBASE_LIBS)       \
       $(OPENLDAP_LIBS)     \
       $(KRB5_LIBS)         \
       $(LWADVAPI_LIBS)     \
       $(PTHREAD_LIBS)
