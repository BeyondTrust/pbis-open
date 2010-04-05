noinst_LTLIBRARIES = liblwadvapi_join.la

liblwadvapi_join_la_SOURCES = \
       globals.c  \
       krbtgt.c \
       lwkrb5.c \
       lwldap.c   \
       lwsecurityidentifier.c   \
       lwtime.c


liblwadvapi_join_la_CPPFLAGS =              \
        -DLW_ENABLE_THREADS=1         \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/include/public \
	$(OPENLDAP_INCLUDES) \
	$(PSTORE_INCLUDES) \
	$(KRB5_INCLUDES)

liblwadvapi_join_la_LIBADD = \
	$(OPENLDAP_LIBS) \
	$(RT_LIBS) \
	$(PSTORE_LIBS) \
	$(KRB5_LIBS)

liblwadvapi_join_la_LDFLAGS = \
	$(OPENLDAP_LDFLAGS) \
	$(PSTORE_LDFLAGS) \
	$(KRB5_LDFLAGS)
