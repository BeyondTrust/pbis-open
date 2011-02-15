AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include

ADDITIONAL_LIBS =
NOINST_DEP_LIBS = \
	@top_builddir@/logging_r/liblsalog_r.la \
	@top_builddir@/server/store/dsapi/libdsapi.la \
        @top_builddir@/server/privilege/liblsaserver_privilege.la

if ENABLE_MINIMAL
noinst_LTLIBRARIES = liblsaserverapi.la
ADDITIONAL_CPPFLAGS = "-DMINIMAL_LSASS=1"
if !ENABLE_SMALL_DAEMON
ADDITIONAL_LIBS += $(NOINST_DEP_LIBS)
endif
else
lib_LTLIBRARIES = liblsaserverapi.la
ADDITIONAL_CPPFLAGS =
ADDITIONAL_LIBS += @EVENTLOG_LIBS@
ADDITIONAL_LIBS += $(NOINST_DEP_LIBS)
ADDITIONAL_LIBS += $(top_builddir)/interop/lwmapsecurity/liblwmapsecurity_internal_lsass.la
endif

liblsaserverapi_la_SOURCES = \
       api2.c          \
       artefacts.c     \
       auth.c          \
       auth_provider.c \
       config.c        \
       credentials.c   \
       event.c         \
       globals.c       \
       ipc_dispatch.c  \
       ipc_error.c     \
       ipc_state.c     \
       lsacfg_reg.c    \
       lsaevent.c      \
       lsatime.c       \
       loginfo.c       \
       machinepwdinfo.c \
       metrics.c       \
       pam.c           \
       provider.c      \
       session.c       \
       state.c         \
       status.c        \
       systemcache.c   \
       traceinfo.c     \
       rpc_server.c    \
       svc_register.c  \
       svc_unregister.c \
       parse_name.c    \
       svc_listen.c

liblsaserverapi_la_CPPFLAGS =     \
	$(AM_CPPFLAGS)                \
	-DLW_ENABLE_THREADS=1         \
	-I@top_srcdir@/server/include \
	$(ADDITIONAL_CPPFLAGS)

liblsaserverapi_la_LIBADD = \
    @UUID_LIBS@ \
    @PTHREAD_LIBS@ \
    $(ADDITIONAL_LIBS) \
    @RT_LIBS@ 
