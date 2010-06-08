AM_CFLAGS = -Wall
AM_CFLAGS += -I$(srcdir)/../include    \
             -I$(top_srcdir)/include   \
             -I$(top_srcdir)/libdomainjoin/include   \
             $(CENTUTILS_INCLUDE)       \
	     $(LWI_OS_FLAGS)		\
             $(GTK_CFLAGS)

AM_CPPFLAGS = -DDATADIR=\"${datadir}\"

AM_LDFLAGS = $(LIBDOMAINJOIN_LDFLAGS) $(LIBDOMAINJOIN_LIBS)
AM_LDFLAGS += $(CENTUTILS_LIBS) $(LIBXML2_LDFLAGS)

bin_PROGRAMS = domainjoin-gtk

domainjoin_gtk_SOURCES = common.c joindialog.c joinprogress.c \
       statusdialog.c joinauth.c joinerror.c main.c

domainjoin_gtk_LDADD = $(top_builddir)/libdomainjoin/libdomainjoin.la $(GTK_LIBS)
