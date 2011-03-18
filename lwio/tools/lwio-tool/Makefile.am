noinst_PROGRAMS = lwio-tool

lwio_tool_SOURCES = \
    lwparseargs.c \
    lwio-tool.c \
    iotest.c \
    srvtest.c

lwio_tool_CPPFLAGS =  \
    -I@top_srcdir@/include

lwio_tool_LDADD = \
    @top_builddir@/client/liblwioclient.la \
    @top_builddir@/common/liblwiocommon.la \
    @top_builddir@/libshareinfo/liblwioshareinfo.la \
    -llwbase_nothr

lwio_tool_LDFLAGS = -L$(top_builddir)/common/.libs \
                    -L$(top_builddir)/libshareinfo/.libs
