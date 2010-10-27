AM_CPPFLAGS = -I$(top_srcdir)/include

noinst_PROGRAMS = testnfs3

testnfs3_SOURCES =   \
    test.c

NFS3_INCLUDES =      \
    -I$(NFS3_INCDIR) \
    -I$(top_srcdir) 

testnfs3_CFLAGS =    \
    $(AM_CPPFLAGS)             \
    $(NFS3_INCLUDES) \
    -D_GNU_SOURCE

testnfs3_LDFLAGS =   \
    -rpath ${libdir} 

#testnfs3_LDADD =     \
#    $(top_builddir)/client/libnfs3.la
