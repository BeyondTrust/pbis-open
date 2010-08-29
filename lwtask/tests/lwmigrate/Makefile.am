bin_PROGRAMS = lwmigrate

lwmigrate_SOURCES = \
    main.c

lwmigrate_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1   \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/server/include

lwmigrate_LDADD = \
    $(top_builddir)/server/lwmigrate/liblwmigrate.la \
    $(top_builddir)/utils/liblwtaskutils.la \
    $(top_builddir)/logging_r/liblwtasklog_r.la

