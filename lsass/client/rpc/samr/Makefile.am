rpccli_srcdir = $(top_srcdir)/client/rpc
idl_srcdir = $(top_srcdir)/idl
lsass_srcdir = $(top_srcdir)

noinst_LTLIBRARIES = libsamr_cs.la libsamr.la

libsamr_la_SOURCES =             \
    globals.c                    \
    samr_binding.c               \
    samr_memory.c                \
    samr_stubmemory.c            \
    samr_close.c                 \
    samr_querysecurity.c         \
    samr_connect2.c              \
    samr_enumdomains.c           \
    samr_enumdomainusers.c       \
    samr_lookupdomain.c          \
    samr_lookupnames.c           \
    samr_opendomain.c            \
    samr_openuser.c              \
    samr_queryuserinfo.c         \
    samr_setuserinfo.c           \
    samr_setuserinfo2.c          \
    samr_createuser.c            \
    samr_deleteuser.c            \
    samr_createdomalias.c        \
    samr_deletedomalias.c        \
    samr_getaliasmembership.c    \
    samr_createdomgroup.c        \
    samr_deletedomgroup.c        \
    samr_opengroup.c             \
    samr_lookuprids.c            \
    samr_queryaliasinfo.c        \
    samr_setaliasinfo.c          \
    samr_addaliasmember.c        \
    samr_deletealiasmember.c     \
    samr_getmembersinalias.c     \
    samr_querydomaininfo.c       \
    samr_enumdomainaliases.c     \
    samr_openalias.c             \
    samr_connect3.c              \
    samr_connect4.c              \
    samr_connect5.c              \
    samr_createuser2.c           \
    samr_getuserpwinfo.c         \
    samr_changepassworduser2.c   \
    samr_getusergroups.c         \
    samr_querydisplayinfo.c

DCEIDL_PASS_INCLUDES = \
    $(CPPFLAGS)        \
    $(LWBASE_INCLUDES) \
    -I$(lsass_srcdir)/include

libsamr_cs_la_SOURCES = samr_cstub.c

libsamr_cs_la_CPPFLAGS =       \
    $(AM_CPPFLAGS)             \
    $(LWBASE_INCLUDES)         \
    $(DCERPC_INCLUDES)         \
    -I$(librpc_srcdir)/include

libsamr_cs_la_CFLAGS =         \
    $(AM_CFLAGS)               \
    -DDCERPC_STUB_BUILD

libsamr_la_CPPFLAGS =          \
    $(AM_CPPFLAGS)             \
    $(LWBASE_INCLUDES)         \
    $(LWADVAPI_INCLUDES)       \
    $(DCERPC_INCLUDES)         \
    $(LIBUNISTR_INCLUDES)      \
    -I$(lsass_srcdir)/include

libsamr_la_CFLAGS = \
    $(AM_CFLAGS)

libsamr_la_LDFLAGS =     \
    $(AM_LDFLAGS)        \
    $(LWBASE_LIBS)       \
    $(LWADVAPI_LIBS)     \
    $(DCERPC_LDFLAGS)    \
    $(LIBUNISTR_LDFLAGS)

libsamr_la_LIBADD =      \
    $(LWBASE_LIBS)       \
    $(LWADVAPI_LIBS)     \
    $(DCERPC_LIBS)       \
    $(LIBUNISTR_LIBS)    \
    libsamr_cs.la

CLEANFILES = samr_h.h samr_cstub.c samr_sstub.c

BUILT_SOURCES = samr_h.h

samr_h.h samr_cstub.c: $(idl_srcdir)/samr.idl samr.acf
	$(IDL) $(IDLFLAGS) -keep c_source -header samr_h.h -cstub_pref cli_ -sstub_pref srv_ \
		-I$(idl_srcdir) -I$(srcdir) $(DCEIDL_PASS_INCLUDES) $<
