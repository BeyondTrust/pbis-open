bin_PROGRAMS = test_authstress

test_authstress_SOURCES = main.c

test_authstress_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	$(LSASS_INCLUDES) \
	$(LWADVAPI_INCLUDES)

test_authstress_LDFLAGS = \
	$(LSASS_LDFLAGS) \
	$(LWADVAPI_LDFLAGS)

test_authstress_LDADD = \
	$(LSASS_LIBS) \
	$(LWADVAPI_NOTHR_LIBS)
