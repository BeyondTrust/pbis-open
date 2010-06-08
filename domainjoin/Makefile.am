ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS=foreign
SUBDIRS=libdomainjoin         \
        domainjoin-cli/src        \
        tests

if BUILD_GTK
SUBDIRS+=domainjoin-gui/gtk
endif

if BUILD_LIBDOMINAJOINMAC
SUBDIRS+=domainjoin-gui/carbon/libdomainjoin-mac/src
endif
