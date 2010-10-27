bin_PROGRAMS = conf2reg

conf2reg_SOURCES = \
    conf2reg.c

conf2reg_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I$(top_srcdir)/include \
    $(SQLITE_INCLUDES)

conf2reg_LDADD = \
    $(top_builddir)/tools/conf2reg/agents/eventlog/libconfeventlog.la \
    $(top_builddir)/tools/conf2reg/agents/lsass/libconflsass.la \
    $(top_builddir)/tools/conf2reg/agents/netlogon/libconfnetlogon.la \
    $(top_builddir)/tools/conf2reg/agents/pstore-sqlite/libpstore-sqlite.la \
    $(top_builddir)/tools/conf2reg/agents/testparse/libconftestparse.la \
    $(top_builddir)/tools/conf2reg/agents/testsambaparse/libconftestsambaparse.la \
    $(top_builddir)/utils/libupgradeutils.la \
    $(SQLITE_LIBS)


