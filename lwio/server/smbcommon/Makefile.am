if ENABLE_STATIC_DRIVERS
if ENABLE_RDR
if !ENABLE_SRV
noinst_LTLIBRARIES = liblwiosmbcommon.la
else
lib_LTLIBRARIES = liblwiosmbcommon.la
endif
else
lib_LTLIBRARIES = liblwiosmbcommon.la
endif
else
lib_LTLIBRARIES = liblwiosmbcommon.la
endif

liblwiosmbcommon_la_SOURCES = \
    wire_close.c         \
    wire_create_dir.c    \
    wire_create_file.c   \
    wire_datetime.c      \
    wire_delete.c        \
    wire_directory.c     \
    wire_check_dir.c     \
    wire_echo.c          \
    wire_error.c         \
    wire_find_close2.c   \
    wire_flush.c         \
    wire_lockx.c         \
    wire_open.c          \
    wire_negotiate.c     \
    wire_ntrename.c      \
    wire_nttransaction.c \
    packet.c             \
    wire_readx.c         \
    wire_rename.c        \
    wire_session_setup.c \
    wire_setinfo.c       \
    wire_setinfo2.c      \
    wire_transaction.c   \
    wire_tree_connect.c  \
    wire_write.c         \
    wire_writex.c        \
    smbkrb5.c

liblwiosmbcommon_la_CPPFLAGS =    \
    -DAPI                         \
    -I@top_srcdir@/include        \
    -I@top_srcdir@/server/include

liblwiosmbcommon_la_LIBADD = \
	@CRYPTO_LIBS@ \
	@RT_LIBS@ \
	@KRB5_LIBS@
