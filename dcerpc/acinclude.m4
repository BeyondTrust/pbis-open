dnl $Revision: 1.2 $

AC_DEFUN([RPC_ARG_DEFINE],
[
AC_ARG_ENABLE($1,
dnl $1=option name
dnl $2=symbol name
dnl $3=if yes, then enable by default
dnl $4=help string
[  --enable-$1		$4 (default=$3)],
[
 case "${enableval}" in
	yes)
		AC_DEFINE($2, 1, [$4])
		rpc_arg_$1=yes
		;;
	no)
		;;
	*)
		AC_MSG_ERROR(bad value ${enableval} for --enable-$1)
		;;
	esac
],
if test "x$3" = "xyes" ; then
	rpc_arg_$1=yes;
	AC_DEFINE($2, 1, [$4])
fi
)
])

dnl Find out where the dcethreads library has been installed
AC_DEFUN([RPC_CHECK_LIBDIR],
dnl RPC_CHECK_LIBDIR(func, library, dirs,action-present,action-notpresent)
[
AC_CACHE_CHECK([for -l$2 in one of $3], [rpc_libdir_$2],
[rpc_func_save_LIBS="$LIBS"
rpc_libdir_$2="no"
AC_TRY_LINK_FUNC([$1], [rpc_libdir_$2="none required"])
if test "$rpc_libdir_$2" = "no"; then
	LIBS="-l$2 $rpc_func_save_LIBS"
	AC_TRY_LINK_FUNC([$1], [rpc_libdir_$2="none required"])
	test "$rpc_libdir_$2" = "no" && for i in $3; do
		LIBS="-L$i/lib -l$2 $rpc_func_save_LIBS"
		AC_TRY_LINK_FUNC([$1],
		[rpc_libdir_$2="$i/lib"
		break])
	done
fi
LIBS="$rpc_func_save_LIBS"])
if test "$rpc_libdir_$2" != "no"; then
	LIBS="-l$2 $LIBS"
	test "$rpc_libdir_$2" = "none required" || {
		LDFLAGS="-L$rpc_libdir_$2 $LDFLAGS"
		AC_MSG_RESULT([found in $rpc_libdir_$2])
	}
	$4
else	:
	$5
fi])

dnl Find out where the dcethreads includes has been installed
AC_DEFUN([RPC_CHECK_INCDIR],
dnl RPC_CHECK_LIBDIR(header, desc, dirs, action-present, action-notpresent)
[
AC_CACHE_CHECK([for $2 header in one of $3], [rpc_incdir_$2],
[rpc_incdir_$2="no"
AC_CHECK_HEADER($1, [rpc_incdir_$2="none required"])
test "$rpc_incdir_$2" = "no" && for i in $3; do
	AC_CHECK_HEADER($i/include/$1,
		[rpc_incdir_$2="$i/include"
		break])
done])
if test "$rpc_incdir_$2" = "no"; then
	unset rpc_incdir_$2
	$5
else
	test "$rpc_incdir_$2" = "none required" || {
		AC_MSG_RESULT([found in $rpc_incdir_$2])
	}
	test "$rpc_incdir_$2" = "none required" && unset rpc_incdir_$2
	$4
fi])

AC_DEFUN([XAC_C_ATTRIBUTE_UNUSED],[
    AC_CACHE_CHECK([name of C compiler's unused attribute], [xac_cv_c_attribute_unused],
    [
	xac_cv_c_attribute_unused=
	for _xac_cv_c_attribute_unused in __attribute__\(\(unused\)\) __unused__ __unused; do
	    AC_TRY_COMPILE(, [} int foo(int arg $_xac_cv_c_attribute_unused) {], [xac_cv_c_attribute_unused=$_xac_cv_c_attribute_unused; break])
	done
    ])
    AC_DEFINE_UNQUOTED([ATTRIBUTE_UNUSED], [$xac_cv_c_attribute_unused], [Name of C compiler's unused attribute])
])
