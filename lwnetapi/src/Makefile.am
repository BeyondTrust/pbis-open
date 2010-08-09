lib_LTLIBRARIES = liblwnetapi.la

liblwnetapi_la_SOURCES = \
    libmain.c                     \
    srvsvc.c                      \
    globals.c                     \
    net_connection.c              \
    net_memory.c                  \
    net_userinfo.c                \
    net_groupinfo.c               \
    net_memberinfo.c              \
    net_displayinfo.c             \
    net_crypto.c                  \
    net_userenum.c                \
    net_useradd.c                 \
    net_userdel.c                 \
    net_user.c                    \
    net_usergetinfo.c             \
    net_usersetinfo.c             \
    net_usergetlocalgroups.c      \
    net_localgroupenum.c          \
    net_localgroupadd.c           \
    net_localgroupdel.c           \
    net_localgroupgetinfo.c       \
    net_localgroupsetinfo.c       \
    net_localgroupmembers.c       \
    net_localgroupgetmembers.c    \
    net_bufferfree.c              \
    net_userchangepassword.c      \
    net_getdomainname.c           \
    net_getdcname.c               \
    net_joindomain.c              \
    net_unjoindomain.c            \
    net_querydisplayinformation.c \
    net_wkstauserenum.c           \
    net_credentials.c

liblwnetapi_la_CPPFLAGS =    \
    $(AM_CPPFLAGS)           \
    -I$(top_srcdir)/include  \
    $(LWADVAPI_INCLUDES)     \
    $(LSASS_INCLUDES)        \
    $(SRVSVC_INCLUDES)       \
    $(DCERPC_INCLUDES)       \
    $(NETLOGON_INCLUDES)

liblwnetapi_la_CFLAGS =      \
    $(AM_CFLAGS)             \
    -Wmissing-prototypes

liblwnetapi_la_LDFLAGS =     \
    $(SRVSVC_LDFLAGS)        \
    $(LSASS_LDFLAGS)         \
    $(LWADVAPI_LDFLAGS)      \
    $(DCERPC_LDFLAGS)        \
    $(NETLOGON_LDFLAGS)

liblwnetapi_la_LIBADD =      \
    $(SRVSVC_LIBS)           \
    $(LSASS_LIBS)            \
    $(LWADVAPI_LIBS)         \
    $(DCERPC_LIBS)           \
    $(NETLOGON_LIBS)
