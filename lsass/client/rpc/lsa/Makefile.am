rpccli_srcdir = $(top_srcdir)/client/rpc
idl_srcdir = $(top_srcdir)/idl
lsass_srcdir = $(top_srcdir)

noinst_LTLIBRARIES = liblsa_cs.la liblsa.la

liblsa_la_SOURCES =                    \
    globals.c                          \
    lsa_binding.c                      \
    lsa_memory.c                       \
    lsa_stubmemory.c                   \
    lsa_openpolicy2.c                  \
    lsa_close.c                        \
    lsa_querysecurity.c                \
    lsa_lookupnames.c                  \
    lsa_lookupnames2.c                 \
    lsa_lookupnames3.c                 \
    lsa_lookupsids.c                   \
    lsa_queryinfopolicy.c              \
    lsa_queryinfopolicy2.c             \
    lsa_enumprivileges.c               \
    lsa_createaccount.c                \
    lsa_openaccount.c                  \
    lsa_enumaccounts.c                 \
    lsa_enumprivilegesaccount.c        \
    lsa_addprivilegestoaccount.c       \
    lsa_removeprivilegesfromaccount.c  \
    lsa_getsystemaccessaccount.c       \
    lsa_setsystemaccessaccount.c       \
    lsa_lookupprivilegevalue.c         \
    lsa_lookupprivilegename.c          \
    lsa_enumaccountswithuserright.c    \
    lsa_enumaccountrights.c            \
    lsa_addaccountrights.c             \
    lsa_removeaccountrights.c"

DCEIDL_PASS_INCLUDES = \
    $(CPPFLAGS)        \
    $(LWBASE_INCLUDES) \
    -I$(lsass_srcdir)/include

liblsa_cs_la_SOURCES = lsa_cstub.c

liblsa_cs_la_CPPFLAGS =          \
    $(AM_CPPFLAGS)               \
    $(LWBASE_INCLUDES)           \
    $(DCERPC_INCLUDES)           \
    -I$(lsass_srcdir)/include

liblsa_cs_la_CFLAGS =    \
    $(AM_CFLAGS)         \
    -DDCERPC_STUB_BUILD

liblsa_la_CPPFLAGS =             \
    $(AM_CPPFLAGS)               \
    $(LWBASE_INCLUDES)           \
    $(DCERPC_INCLUDES)           \
    -I$(lsass_srcdir)/include

liblsa_la_CFLAGS =  \
    $(AM_CFLAGS)

liblsa_la_LDFLAGS =       \
    $(AM_LDFLAGS)         \
    $(LWBASE_LDFLAGS)     \
    $(DCERPC_LDFLAGS)

liblsa_la_LIBADD =     \
    $(LWBASE_LIBS)     \
    $(DCERPC_LIBS)     \
    liblsa_cs.la

CLEANFILES = lsa_h.h lsa_cstub.c lsa_sstub.c

BUILT_SOURCES = lsa_h.h

lsa_cstub.c lsa_h.h: $(idl_srcdir)/lsa.idl lsa.acf
	$(IDL) $(IDLFLAGS) -keep c_source -header lsa_h.h -cstub_pref cli_ -sstub_pref srv_ \
		-I$(idl_srcdir) -I$(srcdir) $(DCEIDL_PASS_INCLUDES) $<
