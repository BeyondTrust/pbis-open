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
# package-mac.sh -- build Mac packages
#
##

### section configure

DEPENDS="core program package"

option()
{
    mk_option \
        OPTION="package-mac" \
        VAR="MK_PACKAGE_MAC" \
        PARAM="yes|no" \
        DEFAULT="yes" \
        HELP="Enable building Mac packages"

    mk_option \
        OPTION="mac-dir" \
        VAR="MK_PACKAGE_MAC_DIR" \
        PARAM="path" \
        DEFAULT="$MK_PACKAGE_DIR/mac" \
        HELP="Subdirectory for built Mac packages"
}

configure()
{
    mk_declare -e MK_PACKAGE_MAC_DIR

    mk_check_program VAR=PACKAGEMAKER \
        /Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker

    if [ "$MK_PACKAGE_MAC" = "yes" -a "$MK_HOST_OS" = "darwin" -a -n "$PACKAGEMAKER" ]
    then
        mk_msg "Mac package building: enabled"
        MK_PACKAGE_MAC_ENABLED=yes
    else
        mk_msg "Mac package building: disabled"
        MK_PACKAGE_MAC_ENABLED=no
    fi
}

#<
# @brief Test if Mac package building is enabled
# @usage
#
# Returns <lit>0</lit> (logical true) if Mac packaging is
# available and was enabled by the user and <lit>1</lit> (logical false)
# otherwise.
#>
mk_macpkg_enabled()
{
    [ "$MK_PACKAGE_MAC_ENABLED" = "yes" ]
}

#<
# @brief Begin Mac package definition
# @usage PACKAGE=name VERSION=ver PMDOC=doc
# @option PACKAGE=name Sets the name of the package
# @option VERSION=ver Sets the version of the package
# @option PMDOC=doc Specifies the PackageMaker document
# that describes the distribution.  The XML files contained
# within will be processed as if by <funcref>mk_output_file</funcref>.
# @option RESOURCES=res_list Specifies a list of resource files
# relative to the directory containing <param>doc</param> that might
# be used.  Each resource will be processed as if by
# <funcref>mk_output_file</funcref>.
# @option BINRESOURCES=bres_list Specifies a list of binary resource
# files relative to the directory containing <param>doc</param>.  Unlike
# <param>res_list</param>, these files will used verbatim without
# processing.
#
# Begins the definition of a Mac distribution file.
# A distribution wraps one or more inner packages.
# You must specify one or more of these inner packages
# with <funcref>mk_macpkg_sub_do</funcref> and
# <funcref>mk_macpkg_sub_done</funcref>.
#
# Use <funcref>mk_macpkg_done</funcref> to complete the definition.
#>
mk_macpkg_do()
{
    mk_push_vars PACKAGE VERSION PMDOC RESOURCES BINRESOURCES
    mk_parse_params
    
    MACPKG_DIR=
    MACPKG_PACKAGE="$PACKAGE"
    MACPKG_VERSION="$VERSION"
    MACPKG_DEPS=""

    mk_resolve_file ".macpkg-${PACKAGE}"
    MACPKG_DIR="$result"
    mk_safe_rm "$MACPKG_DIR"
    mk_mkdir "$MACPKG_DIR"

    MACPKG_SUBPKGS=""

    mk_resolve_file "$PMDOC"
    _pmdoc="$result"
    mk_dirname "$result"
    _pmdoc_dir="$result"

    mk_mkdir "$MACPKG_DIR/dist.pmdoc"
    
    for _xml in "${_pmdoc}"/*.xml
    do
        [ -e "$_xml" ] || mk_fail "invalid PackageMaker doc: $PMDOC"
        mk_basename "$_xml"
        mk_output_file INPUT="@$_xml" OUTPUT="@$MACPKG_DIR/dist.pmdoc/$result"
    done

    mk_unquote_list "$RESOURCES"
    for _res
    do
        mk_resolve_file "$_res"
        result="${result#$_pmdoc_dir/}"
        
        mk_output_file INPUT="$_res" OUTPUT="@$MACPKG_DIR/$result"
    done

    mk_unquote_list "$BINRESOURCES"
    for _res
    do
        mk_resolve_file "$_res"
        _src="$result"
        _dest="$MACPKG_DIR/${result#$_pmdoc_dir/}"

        mk_mkdirname "$_dest"
        mk_run_or_fail cp -R "$_src" "$_dest"
    done

    mk_package_targets()
    {
        mk_quote_list "$@"
        MACPKG_DEPS="$MACPKG_DEPS $result"
        
        for _i
        do
            echo "${_i#@$MK_STAGE_DIR}"
        done >> "${MACPKG_MANIFEST}.files"
    }
    
    mk_package_dirs()
    {
        for _i
        do
            echo "${_i}"
        done >> "${MACPKG_MANIFEST}.dirs"
    }

    mk_pop_vars
}

#<
# @brief Begin Mac subpackage
# @usage SUBPACKAGE=name
# @option SUBPACKAGE=name The name of the subpackage.
# This should be the directory name relative to the PackageMaker
# project where PackageMaker was configured to look for files.
#
# Begins the definition of a Mac package within the
# current distribution being defined.  Use functions such as
# <funcref>mk_package_patterns</funcref> to include files.
#
# Use <funcref>mk_macpkg_sub_done</funcref> to complete the definition.
#>
mk_macpkg_sub_do()
{
    mk_push_vars SUBPACKAGE
    mk_parse_params
    
    MACPKG_SUBPACKAGE="$SUBPACKAGE"
    MACPKG_MANIFEST="$MACPKG_DIR/$SUBPACKAGE"
    MACPKG_SUBPKGS="$MACPKG_SUBPKGS $MACPKG_SUBPACKAGE"
    
    _mk_macpkg_files_begin
    
    mk_pop_vars
}

#<
# @brief End Mac subpackage
# @usage
#
# Ends the subpackage defininition started with
# <funcref>mk_macpkg_sub_do</funcref>.
#>
mk_macpkg_sub_done()
{
    _mk_macpkg_files_end
    
    unset MACPKG_SUBPACKAGE MACPKG_SUBPKGDIR MACPKG_MANIFEST MACPKG_INFOFILES
}

#<
# @brief End Mac package definition
# @usage
#
# Ends a Mac packages definition started
# with <funcref>mk_macpkg_do</funcref>.
#>
mk_macpkg_done()
{
    _mk_macpkg_files_end

    mk_target \
        TARGET="@${MK_PACKAGE_MAC_DIR}/${MACPKG_PACKAGE}" \
        DEPS="$MACPKG_DEPS" \
        mk_run_script build-macpkg '$@' "$MACPKG_VERSION" "$MACPKG_DIR" "*$MACPKG_SUBPKGS"
    master="$result"

    unset MACPKG_PACKAGE MACPKG_VERSION MACPKG_PKGDIR MACPKG_SUBPKGS
    unset -f mk_package_files mk_package_dirs

    mk_add_package_target "$master"

    result="$master"
}

_mk_macpkg_files_begin()
{
    mk_run_or_fail touch "${MACPKG_MANIFEST}.files"
    mk_run_or_fail touch "${MACPKG_MANIFEST}.dirs"
}

_mk_macpkg_files_end()
{
    :
}
