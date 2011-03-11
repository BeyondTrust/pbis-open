bin_PROGRAMS = lwo-adtool

lwo_adtool_SOURCES = \
    main.c
    
lwo_adtool_CPPFLAGS = \
        -I$(top_srcdir)/include

lwo_adtool_LDADD = \
        $(top_builddir)/libadtool/libadtool.la \
        $(POPT_LIBS)
        
lwo_adtool_LDFLAGS = \
        $(POPT_LDFLAGS)
