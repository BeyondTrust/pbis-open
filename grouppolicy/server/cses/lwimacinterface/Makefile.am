gpextnlibdir = @EXTNLIBDIR@
gpextnlib_LTLIBRARIES = liblwimacinterface.la

liblwimacinterface_la_SOURCES = \
    gpplistutils.c \
    lwimaciface_bluetooth.c \
    lwimaciface_energy.c \
    lwimaciface_firewall.c \
    lwimaciface_network.c \
    lwimaciface_security.c \
    lwimacifaceutils.c \
    lwimacinterface.c

#liblwimacinterface_la_CPPFLAGS = \
#    -I@top_srcdir@/server/include

#liblwimacinterface_la_LIBADD = \
#    @top_builddir@/utils/libgputils.la

# Note: .exp file goes with -exported_symbols_list
liblwimacinterface_la_LDFLAGS = \
    -module -export-dynamic -export-symbols $(srcdir)/lwimacinterface.sym -framework Carbon
