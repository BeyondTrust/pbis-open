bin_PROGRAMS = lw-adtool

lw_adtool_SOURCES = \
    main.c
    
lw_adtool_CPPFLAGS = \
        -I$(top_srcdir)/include

lw_adtool_LDADD = \
        $(top_builddir)/libadtool/libadtool.la \
        $(POPT_LIBS)
        
lw_adtool_LDFLAGS = \
        $(POPT_LDFLAGS)
