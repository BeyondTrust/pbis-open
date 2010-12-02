srv_srcdir = $(top_srcdir)/server
rpc_srcdir = $(srv_srcdir)/rpc
idl_srcdir = $(top_srcdir)/idl

if ENABLE_STATIC_RPCSERVERS
noinst_LTLIBRARIES = libsamr_srv.la
else
rpcserver_LTLIBRARIES = libsamr_srv.la
noinst_LTLIBRARIES = libsamr_ss.la libsamr.la
endif

libsamr_la_SOURCES = \
    globals.c                    \
    samr.c                       \
    samr_cfg.c                   \
    samr_srv.c                   \
    samr_memory.c                \
    samr_connect.c               \
    samr_connect2.c              \
    samr_connect3.c              \
    samr_connect4.c              \
    samr_connect5.c              \
    samr_lookupdomain.c          \
    samr_enumdomains.c           \
    samr_opendomain.c            \
    samr_close.c                 \
    samr_setsecurity.c           \
    samr_querysecurity.c         \
    samr_lookupnames.c           \
    samr_lookuprids.c            \
    samr_openaccount.c           \
    samr_openuser.c              \
    samr_openalias.c             \
    samr_enumdomainaccounts.c    \
    samr_enumdomainusers.c       \
    samr_enumdomainaliases.c     \
    samr_queryuserinfo.c         \
    samr_renameaccount.c         \
    samr_setuserinfo.c           \
    samr_setuserinfo2.c          \
    samr_queryaliasinfo.c        \
    samr_setaliasinfo.c          \
    samr_createaccount.c         \
    samr_createuser.c            \
    samr_createuser2.c           \
    samr_createdomalias.c        \
    samr_deleteaccount.c         \
    samr_deletedomalias.c        \
    samr_deleteuser.c            \
    samr_getuserpwinfo.c         \
    samr_rmfromforeigndomain.c   \
    samr_querydisplayinfo.c      \
    samr_querydomaininfo.c       \
    samr_addaliasmember.c        \
    samr_deletealiasmember.c     \
    samr_getmembersinalias.c     \
    samr_getaliasmembership.c    \
    samr_getusergroups.c         \
    samr_changepassworduser2.c   \
    samr_accesstoken.c           \
    samr_security.c              \
    samr_crypto.c                \
    samr_contexthandle.c

DCEIDL_PASS_INCLUDES =        \
    $(CPPFLAGS)               \
    $(LWBASE_INCLUDES)        \
    -I$(top_srcdir)/include

libsamr_la_CFLAGS =         \
    $(AM_CFLAGS)            \
    $(FEATURETEST_FLAGS)    \
    -I$(srv_srcdir)/include \
    -I$(top_srcdir)/include \
    -I$(srv_srcdir)/api     \
    $(DCERPC_INCLUDES)      \
    $(LWBASE_INCLUDES)      \
    $(LWADVAPI_INCLUDES)    \
    $(NETLOGON_INCLUDES)    \
    $(PSTORE_INCLUDES)

libsamr_la_LIBADD =                                \
    $(DCERPC_LIBS)                                 \
    $(LWBASE_LIBS)                                 \
    $(LWADVAPI_LIBS)                               \
    $(NETLOGON_LIBS)                               \
    $(top_builddir)/server/api/liblsaserverapi.la  \
    $(top_builddir)/server/store/dsapi/libdsapi.la \
    $(top_builddir)/common/liblsacommon.la         \
    $(top_builddir)/client/rpc/liblsarpc.la


libsamr_ss_la_SOURCES = \
    samr_sstub.c

libsamr_ss_la_CFLAGS =      \
    $(AM_CFLAGS)            \
    -DDCERPC_STUB_BUILD     \
    $(FEATURETEST_FLAGS)    \
    $(DCERPC_INCLUDES)      \
    $(LWBASE_INCLUDES)

libsamr_ss_la_LIBADD = \
    libsamr.la


libsamr_srv_la_SOURCES =
libsamr_srv_la_CFLAGS = \
    $(AM_CFLAGS)
libsamr_srv_la_LIBADD = \
    libsamr_ss.la
libsamr_srv_la_LDFLAGS = -module -avoid-version

CLEAN_FILES = samr_cstub.c samr_sstub.c samr_h.h

BUILT_SOURCES = samr_h.h

samr_sstub.c samr_cstub.c samr_h.h: $(idl_srcdir)/samr.idl samr.acf
	$(IDL) $(IDLFLAGS) -keep c_source -header samr_h.h -cstub_pref cli_ -sstub_pref srv_ \
                -I$(srcdir) -I$(idl_srcdir) $(DCEIDL_PASS_INCLUDES) $<
