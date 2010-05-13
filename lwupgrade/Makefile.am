ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = \
    $(LWUPGRADE_CPPFLAGS) \
    -I$(top_srcdir)/include \
    $(SQLITE_INCLUDES)

noinst_LTLIBRARIES = \
    utils/libupgradeutils.la

utils_libupgradeutils_la_SOURCES = \
    utils/convert.c \
    utils/format.c \
    utils/machine_account.c \
    utils/parser.c \
    utils/print.c \
    utils/time.c


bin_PROGRAMS = \
    tools/conf2reg/main/conf2reg

tools_conf2reg_main_conf2reg_SOURCES = \
    tools/conf2reg/agents/eventlog/eventlog.c \
    tools/conf2reg/agents/lsass/lsass.c \
    tools/conf2reg/agents/lsass/lsass_global.c \
    tools/conf2reg/agents/lsass/lsass_provider_ad.c \
    tools/conf2reg/agents/lsass/lsass_provider_local.c \
    tools/conf2reg/agents/lsass/pam-config.c \
    tools/conf2reg/agents/lwio/lwio.c \
    tools/conf2reg/agents/netlogon/netlogon.c \
    tools/conf2reg/agents/pstore-sqlite/pstore-sqlite.c \
    tools/conf2reg/agents/testparse/testparse.c \
    tools/conf2reg/agents/testsambaparse/testsambaparse.c \
    tools/conf2reg/main/conf2reg.c

tools_conf2reg_main_conf2reg_LDADD = \
    $(top_builddir)/utils/libupgradeutils.la \
    $(LWBASE_LIBS) \
    $(LWADVAPI_LIBS) \
    $(LWREG_LIBS) \
    $(PSTORE_LIBS) \
    $(SQLITE_LIBS)
