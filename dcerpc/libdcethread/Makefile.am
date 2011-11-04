## Process this file with automake to produce Makefile.in

# Makefile for the dcethreads library and tools
include $(top_srcdir)/include.mk

noinst_LTLIBRARIES = libdcethread.la 

COMMON_FILES = \
	dcethread_create.c \
	dcethread_join.c \
	dcethread_detach.c \
	dcethread_interrupt.c \
	dcethread_pause.c \
	dcethread_cond_wait.c \
	dcethread_atfork.c \
	dcethread_kill.c \
	dcethread_exit.c \
	dcethread_read.c \
	dcethread_write.c \
	dcethread_send.c \
	dcethread_sendto.c \
	dcethread_sendmsg.c \
	dcethread_recv.c \
	dcethread_recvfrom.c \
	dcethread_recvmsg.c \
	dcethread_select.c \
	dcethread_checkinterrupt.c


TEST_FILES = $(COMMON_FILES) dcethread-test.c test-exception.c
API_FILES = $(COMMON_FILES) \
	dcethread-private.c dcethread-debug.c dcethread-util.c dcethread-exception.c \
	dcethread_get_expiration.c \
	dcethread_delay.c \
	dcethread_lock_global.c \
	dcethread_unlock_global.c \
	dcethread_ismultithreaded.c \
	dcethread_mutexattr_getkind.c \
	dcethread_mutexattr_setkind.c \
	dcethread_signal_to_interrupt.c \
	dcethread_attr_create.c \
	dcethread_attr_delete.c \
	dcethread_attr_setprio.c \
	dcethread_attr_getprio.c \
	dcethread_attr_setsched.c \
	dcethread_attr_getsched.c \
	dcethread_attr_setinheritsched.c \
	dcethread_attr_getinheritsched.c \
	dcethread_attr_setstacksize.c \
	dcethread_attr_getstacksize.c \
	dcethread_setprio.c \
	dcethread_getprio.c \
	dcethread_mutexattr_create.c \
	dcethread_mutexattr_delete.c \
	dcethread_mutex_init.c \
	dcethread_mutex_destroy.c \
	dcethread_mutex_lock.c \
	dcethread_mutex_unlock.c \
	dcethread_mutex_trylock.c \
	dcethread_condattr_create.c \
	dcethread_condattr_delete.c \
	dcethread_cond_init.c \
	dcethread_cond_destroy.c \
	dcethread_cond_broadcast.c \
	dcethread_cond_signal.c \
	dcethread_cond_timedwait.c \
	dcethread_once.c \
	dcethread_keycreate.c \
	dcethread_setspecific.c \
	dcethread_getspecific.c \
	dcethread_enableasync.c \
	dcethread_enableinterrupt.c \
	dcethread_yield.c \
	dcethread_equal.c \
	dcethread_self.c \
	dcethread_exc_init.c \
	dcethread_exc_setstatus.c \
	dcethread_exc_getstatus.c \
	dcethread_exc_matches.c \
	dcethread_exc_raise.c \
	dcethread_frame_push.c \
	dcethread_frame_pop.c

libdcethread_la_SOURCES = $(API_FILES)
libdcethread_la_LIBADD = @LIB_pthread@ @LIB_rt@
libdcethread_la_LDFLAGS = -version-info 1:0:0
libdcethread_la_CFLAGS = -Wall -Werror -DAPI

if HAVE_MOONUNIT
lib_LTLIBRARIES = dcethread_mu.la
dcethread_mu_la_SOURCES = $(TEST_FILES) test_stub.c
dcethread_mu_la_LIBADD = libdcethread.la $(LIB_moonunit)
dcethread_mu_la_LDFLAGS = -module -avoid-version -export-dynamic
dcethread_mu_la_CPPFLAGS = -DTEST $(AM_CPPFLAGS)
dcethread_mu_la_CFLAGS = -Wall -Werror

BUILT_SOURCES = test_stub.c

EXTRA_DIST = dcethread-private.h dcethread-debug.h dcethread-util.h

test_stub.c: $(TEST_FILES)
	$(MOONUNIT_STUB) -o $@ CPP="$(CPP)" CPPFLAGS="$(CPPFLAGS) $(AM_CPPFLAGS) -DTEST" $^
endif