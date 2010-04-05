AM_CPPFLAGS = -I$(top_srcdir)/include

noinst_LTLIBRARIES = liblwnetserverapi.la

liblwnetserverapi_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I@top_srcdir@/server/include \
	@PSTORE_INCLUDES@             \
	@LWADVAPI_INCLUDES@           \
	@LWMSG_INCLUDES@              \
	@LWREG_INCLUDES@

liblwnetserverapi_la_SOURCES = \
       globals.c          \
       lwnet-server-cfg.c \
       lwnet-krb5.c       \
       lwnet-cachedb.c    \
       lwnet-pstore.c     \
       lwnet-plugin.c     \
       dcinfo.c           \
       event.c            \
       lwnet.c            \
       state.c

liblwnetserverapi_la_LIBADD = \
	@PSTORE_LIBS@         \
	@LWADVAPI_LIBS@       \
	@PTHREAD_LIBS@        \
	@LWMSG_LIBS@		  \
	@LWREG_LIBS@

liblwnetserverapi_la_LDFLAGS = \
        @PSTORE_LDFLAGS@       \
        @LWADVAPI_LDFLAGS@     \
        @LWREG_LDFLAGS@

noinst_HEADERS = \
		eventdlapi.h           \
		event_p.h              \
		evtstruct.h            \
		lwnet-cachedb.h        \
		lwnet-krb5_p.h         \
		lwnet_p.h              \
		lwnet-pstore_p.h       \
		lwnet-server-cfg_p.h   \
		state_p.h
