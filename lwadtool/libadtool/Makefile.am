lib_LTLIBRARIES = libadtool.la

libadtool_la_SOURCES = \
    api.c \
    cli.c \
    auth.c \
    connect.c \
    delete.c \
    utils.c \
    enable.c \
    unlock.c \
    lookup.c \
    members.c \
    move.c \
    new.c \
    reset_pwd.c \
    search.c \
    error.c \
    ldap_ops.c \
    action_base.c \
    ids.c \
    net.c 
    
AM_CPPFLAGS = -I$(top_srcdir)/include

libadtool_la_CPPFLAGS = \
    $(AM_CPPFLAGS)      \
    $(ETLOGON_INCLUDES) \
    $(OPENLDAP_INCLUDES)    \
    $(LSASS_INCLUDES)    \
    $(LWADVAPI_INCLUDES)

libadtool_la_LIBADD =                    \
    $(LSASS_LIBS)                         \
    $(NETLOGON_LIBS)                      \
    $(OPENLDAP_LIBS)                      \
    $(LWADVAPI_LIBS)                      \
    $(KRB5_LIBS)

libadtool_la_LDFLAGS =                     \
    -module -avoid-version -export-dynamic \
    $(NETLOGON_LDFLAGS)                     \
    $(LSASS_LDFLAGS)                        \
    $(OPENLDAP_LDFLAGS)                     \
    $(LWADVAPI_LDFLAGS)                     \
    $(KRB5_LDFLAGS)

noinst_HEADERS =     \
    error.h \
    defs.h           \
    includes.h
