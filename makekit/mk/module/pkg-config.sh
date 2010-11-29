DEPENDS="core compiler program"

### section configure

mk_pkg_config()
{
    mk_push_vars VARPREFIX libs cppflags cflags ldflags
    mk_parse_params

    if [ -n "$PKG_CONFIG" ] && ${PKG_CONFIG} --exists "$@"
    then
        mk_msg "pkg-config packages $*: yes"
        
        cppflags="`$PKG_CONFIG --cflags-only-I "$@"`"
        [ "$?" -eq 0 ] || mk_fail "pkg-config failed"

        cflags="`$PKG_CONFIG --cflags-only-other "$@"`"
        [ "$?" -eq 0 ] || mk_fail "pkg-config failed"
        
        ldflags="`$PKG_CONFIG --libs-only-other "$@"`"
        [ "$?" -eq 0 ] || mk_fail "pkg-config failed"

        ldflags="$ldflags `$PKG_CONFIG --libs-only-L "$@"`"
        [ "$?" -eq 0 ] || mk_fail "pkg-config failed"

        __libs="`$PKG_CONFIG --libs-only-l "$@"`"
        [ "$?" -eq 0 ] || mk_fail "pkg-config failed"
        
        for __lib in ${__libs}
        do
            libs="$libs ${__lib#-l}"
            _mk_define_name "HAVE_LIB_${__lib#-l}"
            mk_export "$result=external"
        done
        
        mk_msg_verbose "${VARPREFIX} C preprocessor flags: ${cppflags}"
        mk_msg_verbose "${VARPREFIX} compiler flags: $cflags"
        mk_msg_verbose "${VARPREFIX} linker flags: $ldflags"
        mk_msg_verbose "${VARPREFIX} libs: ${libs# }"
        mk_export "${VARPREFIX}_CPPFLAGS"="${cppflags}"
        mk_export "${VARPREFIX}_CFLAGS"="$cflags"
        mk_export "${VARPREFIX}_LDFLAGS"="$ldflags"
        mk_export "${VARPREFIX}_LIBS"="${libs# }"

        __ret=0
    else
        mk_msg "pkg-config packages $*: no"
        __ret=1
    fi

    mk_pop_vars

    return "$__ret"
}

configure()
{
    mk_check_program PROGRAM=pkg-config
}