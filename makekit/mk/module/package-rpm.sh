#
# Copyright (c) Brian Koropoff
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the MakeKit project nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.
#

##
#
# package-rpm.sh -- build RPM packages
#
##

### section configure

DEPENDS="core program package"

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
    mk_declare -e MK_PACKAGE_RPM_DIR

    if mk_check_program PROGRAM=rpmbuild && [ "$MK_PACKAGE_RPM" = "yes" ]
    then
        mk_msg "RPM package building: enabled"
        MK_PACKAGE_RPM_ENABLED=yes
    else
        mk_msg "RPM package building: disabled"
        MK_PACKAGE_RPM_ENABLED=no
    fi
}

#<
# @brief Test if RPM building is enabled
# @usage
#
# Returns <lit>0</lit> (logical true) if RPM packaging is available
# and was enabled by the user and <lit>1</lit> (logical false)
# otherwise.
#>
mk_rpm_enabled()
{
    [ "$MK_PACKAGE_RPM_ENABLED" = "yes" ]
}

#<
# @brief Begin RPM package definition
# @usage PACKAGE=name SPECFILE=specfile
# @option PACKAGE=name Sets the name of the package.
# @option SPECFILE=specfile Designates a template
# RPM spec file to use.
#
# Begins the definition of an RPM package to be built.
# You must provide a template spec file which contains
# basic metadata about the package, such as its name,
# dependencies, and description.  The template should
# omit any sections which control build behavior or
# file lists -- these will be filled in automatically.
#
# After invoking this function, you can use functions
# such as <funcref>mk_package_targets</funcref> or
# <funcref>mk_package_patterns</funcref> to add files
# to the package, or <funcref>mk_rpm_sub_do</funcref>
# to define subpackages.  End the definition of the
# package with <funcref>mk_rpm_done</funcref>.
#>
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

    RPM_SPECFILE="${RPM_PKGDIR}/SPECS/package.spec"
    RPM_RES_SPECFILE="${RPM_RES_PKGDIR}/SPECS/package.spec"
    
    mk_output_file INPUT="$SPECFILE" OUTPUT="$RPM_SPECFILE"
    mk_quote "$result"
    RPM_DEPS="$RPM_DEPS $result"

    # Emit empty clean section to prevent staging directory
    # from being removed
    cat >>"${RPM_RES_SPECFILE}" <<EOF
%clean

EOF
 
    _mk_rpm_files_begin
   
    mk_package_targets()
    {
        mk_quote_list "$@"
        RPM_DEPS="$RPM_DEPS $result"
        
        for _i
        do
            echo "${_i#@$MK_STAGE_DIR}"
        done >> "${RPM_RES_SPECFILE}"
    }
    
    mk_package_dirs()
    {
        # RPM requires the directory
        # to actually exist beforehand.
        # Add a rule to create the directory and
        # add it to the dependency list
        for _i
        do
            mk_target \
                TARGET="$_i" \
                mk_mkdir "&$_i"
            
            mk_quote "$result"
            RPM_DEPS="$RPM_DEPS $result"
        done

        for _i in "$@"
        do
            echo "%dir $_i"
        done >> "${RPM_RES_SPECFILE}"
    }

    mk_pop_vars
}

#<
# @brief Begin RPM subpackage definition
# @usage SUBPACKAGE=name
# @option SUBPACKAGE=name Sets the name of the subpackage
# by itself, e.g. <lit>devel</lit>, <lit>common</lit>.
#
# Begins the definition of an RPM subpackage.  The template
# spec file provided to <funcref>mk_rpm_do</funcref> must
# provide appropriate metadata for the subpackage, but
# the file list will be filled in automatically.
#
# After invoking this function, you can use functions
# such as <funcref>mk_package_targets</funcref> or
# <funcref>mk_package_patterns</funcref> to add files
# to the subpackage.  End the definition of the subpackage
# with <funcref>mk_rpm_sub_done</funcref>.
#>
mk_rpm_sub_do()
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

#<
# @brief End RPM subpackage definition
# @usage
#
# Ends an RPM subpackage definition started
# with <funcref>mk_rpm_sub_do</funcref>.
#>
mk_rpm_sub_done()
{
    unset RPM_SUBPACKAGE RPM_SUBINSTALLFILE RPM_SUBDIRFILE
}
    
#<
# @brief End RPM package definition
# @usage
#
# Ends an RPM package definition started
# with <funcref>mk_rpm_do</funcref>.
#>
mk_rpm_done()
{
    _mk_rpm_files_end

    mk_target \
        TARGET="@${MK_PACKAGE_RPM_DIR}/${RPM_PACKAGE}" \
        DEPS="$RPM_DEPS" \
        _mk_build_rpm "${RPM_PACKAGE}" "&${RPM_PKGDIR}" "&${RPM_SPECFILE}"
    master="$result"

    unset RPM_PACKAGE RPM_SUBPACKAGE RPM_INSTALLFILE RPM_SUBINSTALLFILE RPM_PKGDIR
    unset RPM_SUBPACKAGES
    unset -f mk_package_files mk_package_dirs

    mk_add_package_target "$master"

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
