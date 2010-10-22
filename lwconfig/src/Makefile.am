bin_PROGRAMS = lwconfig

lwconfig_SOURCES = \
    capability.c \
    main.c \
    util.c

lwconfig_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I$(top_srcdir)/include \
    $(XML2_INCLUDES) \
    $(LWBASE_INCLUDES) \
    $(LWADVAPI_LIBS) \
    $(LWREG_LIBS)

lwconfig_LDADD = \
    $(XML2_LIBS) \
    $(LWBASE_LIBS) \
    $(LWADVAPI_LIBS) \
    $(LWREG_LIBS)
