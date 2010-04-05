bin_PROGRAMS = lwregshell
bin_SCRIPTS = lw-edit-reg

#lwregshell
lwregshell_SOURCES = \
    cmdparse.c \
    regshell.c

lwregshell_LDADD = \
        @top_builddir@/shellutil/librsutils.la \
        @top_builddir@/utils/libregutils.la \
        @top_builddir@/parse/libregparse.la \
        @top_builddir@/client/libregclient.la \
        @top_builddir@/libedit/src/libedit.la \
        @LWBASE_NOTHR_LIBS@ \
        @LWMSG_NOTHR_LIBS@

lwregshell_CPPFLAGS = \
    $(AM_CPPFLAGS)   \
    -I@top_srcdir@/shellutil \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/libedit/src \
    @LWBASE_INCLUDES@

lw-edit-reg: lw-edit-reg.sh
	cp $< $@
	chmod +x $@
