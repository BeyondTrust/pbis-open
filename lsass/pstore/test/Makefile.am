AM_CPPFLAGS += -I$(srcdir)/../../tools/lw-lsa

noinst_PROGRAMS = lsapstore-test

lsapstore_test_SOURCES =  \
        lsapstore-test.c

lsapstore_test_LDADD = \
        $(LWBASE_LIBS) \
        $(LWADVAPI_LIBS) \
        ../liblsapstore.la
