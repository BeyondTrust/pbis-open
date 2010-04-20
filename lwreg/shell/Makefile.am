bin_PROGRAMS = lwregshell
bin_SCRIPTS = lw-edit-reg

#lwregshell
lwregshell_SOURCES = \
    cmdparse.c \
    regshell.c

lwregshell_LDADD = \
        @top_builddir@/shellutil/librsutils.la \
        @top_builddir@/parse/libregparse.la \
        @top_builddir@/libedit/src/libedit.la

lwregshell_CPPFLAGS = \
    $(AM_CPPFLAGS)   \
    -I@top_srcdir@/shellutil \
    -I@top_srcdir@/libedit/src \
    @LWBASE_INCLUDES@

lw-edit-reg: lw-edit-reg.sh
	cp $< $@
	chmod +x $@
