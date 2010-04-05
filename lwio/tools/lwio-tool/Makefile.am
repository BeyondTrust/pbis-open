noinst_PROGRAMS = lwio-tool

lwio_tool_SOURCES = \
    lwparseargs.c \
    lwio-tool.c \
    iotest.c

lwio_tool_CPPFLAGS =  \
    -I@top_srcdir@/include

lwio_tool_LDADD = \
    @top_builddir@/client/liblwioclient.la \
    @LWBASE_LIBS@

lwio_tool_LDFLAGS = -L$(top_builddir)/common/.libs
