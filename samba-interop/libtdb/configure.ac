AC_INIT(libtdb, 4.0, support@likewisesoftware.com)

AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_MACRO_DIR([m4])

AC_DEFUN([SMB_MODULE_DEFAULT], [echo -n ""])
AC_DEFUN([SMB_LIBRARY_ENABLE], [echo -n ""])
AC_DEFUN([SMB_ENABLE], [echo -n ""])
AC_CONFIG_HEADER(include/config.h)

AC_PROG_CC
AC_PROG_LIBTOOL

AC_SYS_LARGEFILE
AC_CHECK_SIZEOF(off_t,cross)
AC_CHECK_SIZEOF(size_t,cross)
AC_CHECK_SIZEOF(ssize_t,cross)
AC_FUNC_MMAP

AC_CHECK_FUNCS([mmap pread pwrite getpagesize utime])
AC_CHECK_HEADERS([getopt.h sys/select.h sys/time.h])

AC_CHECK_DECLS([pread pwrite], , , [#include <unistd.h>])

AC_CHECK_HEADERS([standards.h stdint.h inttypes.h string.h strings.h sys/types.h dlfcn.h limits.h stdbool.h sys/param.h sys/wait.h setjmp.h sys/ipc.h sys/shm.h sys/mman.h fcntl.h sys/fcntl.h])
AC_CHECK_FUNCS([strerror mktime strdup memmove mktime timegm strlcpy strlcat strndup strnlen setenv unsetenv seteuid setegid setlinebuf strcasestr strtok_r strtoll strtoull ftruncate initgroups bzero memset dlerror dlopen dlsym dlclose socketpair vasprintf snprintf vsnprintf asprintf vsyslog va_copy __va_copy strptime mkdtemp])
AC_CHECK_TYPES([comparison_fn_t, bool, Bool, sig_atomic_t], , , [
#if HAVE_STDBOOL_H
#include <stdbool.h>
#endif
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#include <signal.h>])
AC_CHECK_DECL([errno setenv])

AM_CPPFLAGS=
AC_SUBST(AM_CPPFLAGS)

sinclude(config.m4)
AC_OUTPUT(Makefile tdb.pc)
