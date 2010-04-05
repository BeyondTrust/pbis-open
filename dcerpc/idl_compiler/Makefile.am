## Process this file with automake to produce Makefile.in
include $(top_srcdir)/include.mk

# This Makefile builds the IDL compiler

# configure/automake adds the dcethread library to everything that is built in
# this package.  Most things need it, but the idl compiler doesn't, so we
# remove the dcethread library from LIBS here.  You might not expect
# -ldcethread to stick during linking, but it does because it includes
# implementations of some io functions.
#LIBS=$(subst -ldcethread,,$(LIBS))

AM_CFLAGS+=-DMIA -DDEFAULT_IDIR=\"$(includedir)\" -DCATALOG_DIR=\"$(pkgdatadir)\" -DYYERROR_VERBOSE=1 -DYYDEBUG=1

bin_PROGRAMS=dceidl

pkgdata_DATA=idl.cat

dceidl_SOURCES = acf_y.c acf_l.c nidl_y.c nidl_l.c astp_com.c astp_cpx.c \
	astp_gbl.c astp_sim.c astp_dmp.c checker.c chkichar.c command.c \
	driver.c errors.c files.c frontend.c getflags.c keywds.c main.c \
	message.c nametbl.c namdump.c propagat.c sysdep.c mtsbacke.c \
	ddbe.c ddspell.c cspeldcl.c cspell.c cstubmts.c hdgen.c clihamts.c \
	mtspipes.c irepgen.c irepscp.c ifspemts.c sstubmts.c comstmts.c \
	user_exc.c icharsup.c astp_exp.c
dceidl_LDFLAGS = @LTLDFLAGS@

noinst_HEADERS= acf.h ast.h astp.h astp_dmp.h backend.h be_pvt.h bedeck.h \
 checker.h chkichar.h clihamts.h clihandl.h command.h commstat.h cspeldcl.h \
 cspell.h cstubgen.h cstubmts.h ddbe.h default_msg.h driver.h dutils.h \
 errors.h fe_pvt.h files.h flex_bison_support.h frontend.h genpipes.h \
 getflags.h hdgen.h icharsup.h ifspec.h ir_pvt.h irep.h marshall.h \
 message.h mtsbacke.h mtspipes.h nametbl.h namtbpvt.h nidl.h nidlmsg.h \
 propagat.h sstubmts.h sysdep.h user_exc.h

BUILT_SOURCES=acf_y.c acf_l.c nidl_y.c nidl_l.c idl.cat

# Override the standard YACC/LEX rules so we can build two scanners into
# the idl compiler.

acf_y.c acf_y.h: acf_y.y
	$(BISON) -v $(AM_YFLAGS) $(YFLAGS) -p acf_yy -o acf_y.c -d $<

acf_l.c: acf_l.l
	$(FLEX) $(AM_LFLAGS) $(LFLAGS) -Pacf_yy -o$@ $<

nidl_y.c nidl_y.h: nidl_y.y
	$(BISON) -v $(AM_YFLAGS) $(YFLAGS) -p nidl_yy -o nidl_y.c -d $<

nidl_l.c: nidl_l.l
	$(FLEX) $(AM_LFLAGS) $(LFLAGS) -Pnidl_yy -o$@ $<

# Generate error message catalog for IDL compiler
idl.cat default_msg.h:	nidlmsg.m
	$(RM) $(RMFLAGS) default_msg.h idl.cat
	$(SED) -e '/^\$$/d;/^$$/d;s/^[^ ]* /"/;s/$$/",/;' $(srcdir)/nidlmsg.m > default_msg.h
	$(GENCAT) idl.cat $(srcdir)/nidlmsg.m

# Keep the generated parsers in the tarball
EXTRA_DIST=nidlmsg.m acf_y.c acf_y.h acf_l.c nidl_y.c nidl_y.h nidl_l.c

# Ensure that YACC/LEX generated files go away during clean
CLEANFILES=acf_y.c acf_l.c acf_y.h nidl_y.c nidl_l.c nidl_y.h

# TODO-dalmeida-2008/01/22-This DESTDIR/install-exec-local stuff looks suspicious
DESTDIR = /

install-exec-hook:
	test -f $(DESTDIR)/$(bindir)/idl || $(LN_S) dceidl $(DESTDIR)/$(bindir)/idl

