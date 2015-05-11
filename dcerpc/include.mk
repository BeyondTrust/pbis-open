dce_includes=-I$(top_srcdir)/include -I$(top_builddir)/include @DCETHREADINCLUDES@

BASE_CPPFLAGS=$(dce_includes)
BASE_CFLAGS=-g -Wall -O -pipe
#BASE_CFLAGS+=-Werror

AM_CPPFLAGS=$(BASE_CPPFLAGS)
AM_CFLAGS=$(BASE_CFLAGS)

SUFFIXES=.idl

# TODO-2008/0124-dalmeida - More correct naming of variable.
IDL_CPPFLAGS=$(IDL_CFLAGS)

IDL=$(top_builddir)/idl_compiler/dceidl
IDL_INCLUDE_DIR=$(top_srcdir)/include/dce

IDLFLAGS=$(IDL_CFLAGS) -cepv -client none -server none -I$(IDL_INCLUDE_DIR)/.. $(IDL_FLAGS)
NCK_IDLFLAGS=-keep object -no_cpp -v -no_mepv -I$(IDL_INCLUDE_DIR)/.. -I$(top_builddir)/include @DCETHREADINCLUDES@ $(TARGET_OS) -cc_cmd '$(LIBTOOL) --mode=compile $(IDL_CC) -c $(IDL_CFLAGS) '

MODULELDFLAGS=-module -avoid-version -export-dynamic

LTCOMPILE=$(LIBTOOL) --mode=compile $(CC) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
LINK=$(LIBTOOL) --mode=link $(CCLD) $(AM_CFLAGS) $(CFLAGS) $(LDFLAGS) -o $@

# ylwrap is provided by automake as a wrapper to allow multiple invocations in
# a single directory.

YLWRAP = $(top_srcdir)/build/ylwrap

%.h: %.idl
	$(IDL) $(IDLFLAGS) -no_mepv $<

# Create default message strings from a msg file
#%_defmsg.h:	%.msg
#	echo $(RM) $(RMFLAGS) $@
#	echo $(SED) -e '/^\$$/d;/^$$/d;s/^[^ ]* /"/;s/$$/",/;' $< > $@

#%.cat:	%.msg
#	$(RM) $(RMFLAGS) $@
#	$(GENCAT) $@ $<
