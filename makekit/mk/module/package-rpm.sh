DEPENDS="core program package"

### section configure

# API
# mk_rpm_do
#   mk_subpackage_do
#     mk_package_files
#     mk_package_dirs
#   mk_subpackage_done
# mk_rpm_done

option()
{
    mk_option \
        OPTION="package-rpm" \
        VAR="MK_PACKAGE_RPM" \
        PARAM="yes|no" \
        DEFAULT="yes" \
        HELP="Enable building RPM packages"

    mk_option \
        OPTION="rpm-dir" \
        VAR="MK_PACKAGE_RPM_DIR" \
        PARAM="path" \
        DEFAULT="$MK_PACKAGE_DIR/rpm" \
        HELP="Subdirectory for built RPM packages"
}

configure()
{
    mk_export MK_PACKAGE_RPM_DIR

    if mk_check_program PROGRAM=rpmbuild && [ "$MK_PACKAGE_RPM" = "yes" ]
    then
        mk_msg "RPM package building: enabled"
        mk_export MK_PACKAGE_RPM_ENABLED=yes
    else
        mk_msg "RPM package building: disabled"
        mk_export MK_PACKAGE_RPM_ENABLED=no
    fi
}

mk_rpm_enabled()
{
    [ "$MK_PACKAGE_RPM_ENABLED" = "yes" ]
}

mk_rpm_do()
{
    mk_push_vars PACKAGE SPECFILE VERSION
    mk_parse_params
    
    RPM_PKGDIR=".rpm-${PACKAGE}"
    RPM_PACKAGE="$PACKAGE"
    RPM_VERSION="$VERSION"
    RPM_SUBPACKAGES=""
    RPM_SUBPACKAGE=""
    RPM_DEPS=""

    mk_resolve_file "$RPM_PKGDIR"
    RPM_RES_PKGDIR="$result"
    mk_safe_rm "$RPM_RES_PKGDIR"

    for i in BUILD RPMS SOURCES SPECS SRPMS
    do
        mk_mkdir "$RPM_RES_PKGDIR/$i"
    done

    RPM_SPECFILE="${RPM_PKGDIR}/SPECS/${SPECFILE##*/}"
    RPM_RES_SPECFILE="${RPM_RES_PKGDIR}/SPECS/${SPECFILE##*/}"
    
    mk_output_file INPUT="$SPECFILE" OUTPUT="$RPM_SPECFILE"
    mk_quote "$result"
    RPM_DEPS="$RPM_DEPS $result"

    # Emit empty clean section to prevent staging directory
    # from being removed
    cat >>"${RPM_RES_SPECFILE}" <<EOF
%clean

EOF
 
    _mk_rpm_files_begin
   
    mk_subpackage_do()
    {
        mk_push_vars SUBPACKAGE
        mk_parse_params

        [ -z "$SUBPACKAGE" ] && SUBPACKAGE="$1"
        RPM_SUBPACKAGE="$SUBPACKAGE"
        RPM_SUBPACKAGES="$RPM_SUBPACKAGES $SUBPACKAGE"

        _mk_rpm_files_end
        _mk_rpm_files_begin "$SUBPACKAGE"

        mk_pop_vars
    }

    mk_subpackage_done()
    {
        unset RPM_SUBPACKAGE RPM_SUBINSTALLFILE RPM_SUBDIRFILE
    }

    mk_package_files()
    {
        for _i in "$@"
        do
            echo "$_i"
        done >> "${RPM_RES_SPECFILE}"
    }
    
    mk_package_dirs()
    {
        # RPM requires the directory
        # to actually exist beforehand.
        # This is kind of hacky, but just
        # create it now.
        for _i
        do
            mk_resolve_file "$_i"
            mk_mkdir "$result"
        done

        for _i in "$@"
        do
            echo "%dir $_i"
        done >> "${RPM_RES_SPECFILE}"
    }

    mk_pop_vars
}

mk_rpm_done()
{
    _mk_rpm_files_end

    mk_target \
        TARGET="@${MK_PACKAGE_RPM_DIR}/${RPM_PACKAGE}" \
        DEPS="$RPM_DEPS @all" \
        _mk_build_rpm "${RPM_PACKAGE}" "&${RPM_PKGDIR}" "&${RPM_SPECFILE}"
    master="$result"

    mk_add_phony_target "$master"
    mk_add_subdir_target "$master"

    unset RPM_PACKAGE RPM_SUBPACKAGE RPM_INSTALLFILE RPM_SUBINSTALLFILE RPM_PKGDIR
    unset RPM_SUBPACKAGES
    unset -f mk_package_files mk_package_dirs mk_subpackage_do mk_subpackage_done

    result="$master"
}

_mk_rpm_files_begin()
{
    {
        echo "%files $1"
        echo "%defattr(-,root,root)"
    } >> "${RPM_RES_SPECFILE}"
}

_mk_rpm_files_end()
{
    printf "\n" >> "${RPM_RES_SPECFILE}"
}

### section build

_mk_build_rpm()
{
    # $1 = package name
    # $2 = build directory
    # $3 = spec file

    mk_msg_domain "rpm"

    mk_safe_rm "${MK_PACKAGE_RPM_DIR}/$1"
    mk_mkdir "${MK_PACKAGE_RPM_DIR}/$1"
    mk_msg "begin $1"
    mk_run_quiet_or_fail ${RPMBUILD} \
        --define "_topdir ${MK_ROOT_DIR}/${2}" \
        --define "_unpackaged_files_terminate_build 0" \
        --define "_prefix ${MK_PREFIX}" \
        --define "_exec_prefix ${MK_EPREFIX}" \
        --define "_bindir ${MK_BINDIR}" \
        --define "_sbindir ${MK_SBINDIR}" \
        --define "_sysconfdir ${MK_SYSCONFDIR}" \
        --define "_datadir ${MK_DATADIR}" \
        --define "_includedir ${MK_INCLUDEDIR}" \
        --define "_libdir ${MK_LIBDIR}" \
        --define "_libexecdir ${MK_LIBEXECDIR}" \
        --define "_localstatedir ${MK_LOCALSTATEDIR}" \
        --define "_mandir ${MK_MANDIR}" \
        --buildroot="${MK_ROOT_DIR}/${MK_STAGE_DIR}" \
        -bb "$3"
    mk_run_or_fail mv "${2}/RPMS"/*/*.rpm "${MK_PACKAGE_RPM_DIR}/$1/."
    for i in "${MK_PACKAGE_RPM_DIR}/$1"/*.rpm
    do
        mk_msg "built ${i##*/}"
    done
    mk_msg "end $1"
}
