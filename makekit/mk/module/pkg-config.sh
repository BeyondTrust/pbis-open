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

        __cflags="`$PKG_CONFIG --cflags-only-other "$@"`"
        [ "$?" -eq 0 ] || mk_fail "pkg-config failed"
        
        cflags=""
        for __cflag in ${__cflags}
        do
            # FIXME: only do this for gcc-like compilers
            case "$__cflag" in
                "-mt")
                    cflags="$cflags -pthread"
                    ;;
                *)
                    cflags="$cflags $__cflag"
                    ;;
            esac
        done

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
            mk_declare -i "$result=external"
        done
        
        mk_msg_verbose "${VARPREFIX} C preprocessor flags: ${cppflags}"
        mk_msg_verbose "${VARPREFIX} compiler flags: $cflags"
        mk_msg_verbose "${VARPREFIX} linker flags: $ldflags"
        mk_msg_verbose "${VARPREFIX} libs: ${libs# }"
        mk_declare -i "${VARPREFIX}_CPPFLAGS"="${cppflags}"
        mk_declare -i "${VARPREFIX}_CFLAGS"="$cflags"
        mk_declare -i "${VARPREFIX}_LDFLAGS"="$ldflags"
        mk_declare -i "${VARPREFIX}_LIBS"="${libs# }"

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