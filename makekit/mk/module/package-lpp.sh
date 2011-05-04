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
# package-lpp.sh -- build AIX packages
#
##

### section configure

DEPENDS="core program package"

option()
{
    mk_option \
        OPTION="package-lpp" \
        VAR="MK_PACKAGE_LPP" \
        PARAM="yes|no" \
        DEFAULT="yes" \
        HELP="Enable building AIX packages"

    mk_option \
        OPTION="lpp-dir" \
        VAR="MK_PACKAGE_LPP_DIR" \
        PARAM="path" \
        DEFAULT="$MK_PACKAGE_DIR/lpp" \
        HELP="Subdirectory for built AIX packages"
}

configure()
{
    mk_declare -e MK_PACKAGE_LPP_DIR

    if [ "$MK_PACKAGE_LPP" = "yes" -a "$MK_HOST_OS" = "aix" ]
    then
        mk_msg "AIX package building: enabled"
        MK_PACKAGE_LPP_ENABLED=yes
    else
        mk_msg "AIX package building: disabled"
        MK_PACKAGE_LPP_ENABLED=no
    fi
}

#<
# @brief Test if AIX package building is enabled
# @usage
#
# Returns <lit>0</lit> (logical true) if AIX packaging is
# available and was enabled by the user and <lit>1</lit> (logical false)
# otherwise.
#>
mk_lpp_enabled()
{
    [ "$MK_PACKAGE_LPP_ENABLED" = "yes" ]
}

_mk_lpp_process_infofiles()
{
    mk_unquote_list "$INFOFILES"
    for _info
    do
        _dest="${LPP_SUBLPPDIR}/info/${_info##*/}"
        _dest="${_dest%.in}"
        mk_output_file INPUT="$_info" OUTPUT="@$_dest"
    done
}

#<
# @brief Begin AIX package definition
# @usage PACKAGE=name VERSION=ver
# @option PACKAGE=name Sets the name of the package.  AIX package
# names are a hierarchy of dot-separated components, usually of the
# form <lit>vendor.product</lit>.  If the <param>name</param> contains hyphens,
# as is typical of other package managers, they will be converted to
# dots.
# @option VERSION=ver Sets the version of the package.  AIX package versions
# consist of 4 dot-separated numeric components.
#
# Begins the definition of an AIX LPP package file (<lit>.bff</lit>).
# You must define one or more filesets with <funcref>mk_lpp_sub_do</funcref>.
#
# Use <funcref>mk_lpp_done</funcref> to complete the definition.
#>
mk_lpp_do()
{
    mk_push_vars PACKAGE VERSION
    mk_parse_params
    
    LPP_PACKAGE="$PACKAGE"
    LPP_PACKAGEDOT=`echo "$PACKAGE" | tr '-' '.'`
    LPP_VERSION="$VERSION"
    LPP_DEPS=""
    LPP_SUBLPPS=""

    mk_resolve_file ".lpp-${LPP_PACKAGE}"
    LPP_LPPDIR="$result"
    mk_safe_rm "$LPP_LPPDIR"
    mk_mkdir "$LPP_LPPDIR"

    mk_package_targets()
    {
        mk_quote_list "$@"
        LPP_DEPS="$LPP_DEPS $result"
        
        for _i
        do
            _i="${_i#@$MK_STAGE_DIR}"
            case "$_i" in
                /usr/*|/opt/*)
                    echo "$_i" >> "${LPP_MANIFEST}.usr.files"
                    continue
                    ;;
            esac
            echo "$_i" >> "${LPP_MANIFEST}.root.files"
        done
    }
    
    mk_package_dirs()
    {
        for _i
        do
            case "$_i" in
                /usr/*|/opt/*)
                    echo "$_i" >> "${LPP_MANIFEST}.usr.dirs"
                    continue
                    ;;
            esac
            echo "$_i" >> "${LPP_MANIFEST}.root.dirs"
        done
    }

    mk_pop_vars
}

#<
# @brief Begin AIX fileset
# @usage SUBPACKAGE=name DESCRIPTION=desc
# @option SUBPACKAGE=name Sets the name of the fileset.
# This will be appended to the name of the overall package.
# @option VERSION=ver Sets the version of the fileset.  Defaults
# to the version specified for the overall package.
# @option DESCRIPTION=desc Sets a brief English-language
# description.
# @option PREIN=prein Indicates a script file that should be
# run before installing the fileset.  All scripts are processed
# as by <funcref>mk_output_file</funcref>.
# @option UNPREIN=unprein Indicates a script file that should
# be run to revert the effects of the <param>prein</param> script.
# @option POSTIN=postin Indicates a script file that should
# be run after installing the fileset.
# @option UNPOSTIN=unpostin Indicates a script file that should
# be run to revert the effects of the <param>postin</param> script.
# @option PRERM=prerm Indicates a script that should be run before
# removing a previous version of the fileset.
# @option CONFIG=config Indicates a script that should be run when
# configuring the fileset.
# @option UNCONFIG=unconfig Indicates a script that should be run when
# unconfiguring the fileset.
# @option REQUISITES=reqfile Specifies an optional requisites file
# that describes the dependencies of the fileset.
# @option UPSIZE=upspec Specifies a list of directories and how much
# space in 512-byte blocks might need to be allocated in order to
# install the fileset, e.g. <lit>"/opt 2048"</lit>.
#
# Begins the definition of a fileset within an AIX package.
# Use functions such as <funcref>mk_package_patterns</funcref> to
# include files in the package.
#
# By default, intermediate directories for included files
# are assumed to exist on the target system to avoid potentially
# overriding their permissions.  If you install files into directories
# that are not pre-existing on the system, you should specify
# them with <funcref>mk_package_dirs</funcref>.
#
# Use <funcref>mk_lpp_sub_done</funcref> to complete the definition.
#>
mk_lpp_sub_do()
{
    mk_push_vars \
        SUBPACKAGE="rte" \
        VERSION="$LPP_VERSION" \
        DESCRIPTION="None" \
        PREIN UNPREIN POSTIN UNPOSTIN PRERM \
        CONFIG UNCONFIG REQUISITES UPSIZE
        
    mk_parse_params

    LPP_SUBPACKAGE="${SUBPACKAGE}"
    LPP_MANIFEST="${LPP_LPPDIR}/$SUBPACKAGE"
    LPP_SUBLPPS="$LPP_SUBLPPS $SUBPACKAGE"

    echo "${LPP_PACKAGEDOT}.${SUBPACKAGE} ${VERSION} 01 N B ${LANG} $DESCRIPTION" > "$LPP_MANIFEST.name" || mk_fail "could not write manifest"

    [ -n "$PREIN" ] &&
    mk_output_file INPUT="$PREIN" OUTPUT="@$LPP_MANIFEST.pre_i"
    [ -n "$UNPREIN" ] &&
    mk_output_file INPUT="$UNPREIN" OUTPUT="@$LPP_MANIFEST.pre_u"
    [ -n "$POSTIN" ] &&
    mk_output_file INPUT="$POSTIN" OUTPUT="@$LPP_MANIFEST.post_i"
    [ -n "$UNPOSTIN" ] &&
    mk_output_file INPUT="$UNPOSTIN" OUTPUT="@$LPP_MANIFEST.post_u"
    [ -n "$PRERM" ] &&
    mk_output_file INPUT="$PRERM" OUTPUT="@$LPP_MANIFEST.pre_rm"
    [ -n "$CONFIG" ] &&
    mk_output_file INPUT="$CONFIG" OUTPUT="@$LPP_MANIFEST.config"
    [ -n "$UNCONFIG" ] &&
    mk_output_file INPUT="$UNCONFIG" OUTPUT="@$LPP_MANIFEST.unconfig"
    [ -n "$REQUISITES" ] &&
    mk_output_file INPUT="$REQUISITES" OUTPUT="@$LPP_MANIFEST.requisites"
    echo "$UPSIZE" > "$LPP_MANIFEST.upsize" || mk_fail "could not write manifest"

    _mk_lpp_files_begin
    
    mk_pop_vars
}

#<
# @brief End AIX subpackage
# @usage
#
# Ends the subpackage defininition started with
# <funcref>mk_lpp_sub_do</funcref>.
#>
mk_lpp_sub_done()
{
    _mk_lpp_files_end
    
    unset LPP_SUBPACKAGE LPP_MANIFEST
}

#<
# @brief End AIX package definition
# @usage
#
# Ends a AIX packages definition started
# with <funcref>mk_lpp_do</funcref>.
#>
mk_lpp_done()
{
    mk_target \
        TARGET="@${MK_PACKAGE_LPP_DIR}/${LPP_PACKAGE}" \
        DEPS="$LPP_DEPS" \
        mk_run_script build-lpp \
        '$@' "$LPP_VERSION" "$LPP_LPPDIR" "*$LPP_SUBLPPS"
    master="$result"

    unset LPP_PACKAGE LPP_VERSION LPP_LPPDIR LPP_SUBLPPS
    unset -f mk_package_files mk_package_dirs

    mk_add_package_target "$master"

    result="$master"
}

_mk_lpp_files_begin()
{
    mk_run_or_fail touch "${LPP_MANIFEST}.usr.files"
    mk_run_or_fail touch "${LPP_MANIFEST}.usr.dirs"
    mk_run_or_fail touch "${LPP_MANIFEST}.root.files"
    mk_run_or_fail touch "${LPP_MANIFEST}.root.dirs"
}

_mk_lpp_files_end()
{
    :
}
