DEPENDS="program core"

# section configure

lw_check_dceidl()
{
    mk_check_program dceidl
}

lw_dceidl()
{
    mk_push_vars IDL HEADER CSTUB SSTUB IDLFLAGS INCLUDEDIRS HEADERDEPS
    mk_parse_params

    unset _deps

    mk_resolve_target "$IDL"
    _idl="$result"

    if [ -z "$DCEIDL" -o "$DCEIDL" = "${MK_RUN_BINDIR}/dceidl" ]
    then
        # If we built dceidl ourselves (it is in the run bindir),
        # make sure we have a dependency on it
        _deps="$_deps '@${MK_RUN_BINDIR}/dceidl'"
    fi

    for _header in ${HEADERDEPS}
    do
        if _mk_contains "$_header" ${MK_INTERNAL_HEADERS}
        then
            _deps="$_deps '${MK_INCLUDEDIR}/${_header}'"
        fi
    done
    
    mk_target \
        TARGET="$HEADER" \
        DEPS="'$_idl' ${_deps}" \
        mk_run_script dceidl %HEADER %CSTUB %SSTUB %IDLFLAGS %INCLUDEDIRS %DCEIDL "$_idl"

    _header="$result"

    if [ -n "$CSTUB" ]
    then
        mk_target \
            TARGET="$CSTUB" \
            DEPS="'$_header'"
    fi

    if [ -n "$SSTUB" ]
    then
        mk_target \
            TARGET="$SSTUB" \
            DEPS="'$_header'"
    fi

    mk_pop_vars
}

