bin_PROGRAMS = lwmigrate

lwmigrate_SOURCES  = \
    main.c

lwmigrate_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1   \
    -I@top_srcdir@/include

lwmigrate_LDADD    = $(top_builddir)/client/liblwtaskclient.la

