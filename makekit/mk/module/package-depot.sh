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
# package-depot.sh -- build HP-UX packages
#
##

DEPENDS="core program package"

### section common

_depot_name()
{
    _IFS="$IFS"
    IFS="-"
    set -- $1
    IFS="$_IFS"
    result=""
    for part
    do
        rest="${part#?}"
        first=`echo "${part%$rest}" | tr '[:lower:]' '[:upper:]'`
        result="$result$first$rest"
    done
}

### section configure


option()
{
    mk_option \
        OPTION="package-depot" \
        VAR="MK_PACKAGE_DEPOT" \
        PARAM="yes|no" \
        DEFAULT="yes" \
        HELP="Enable building HP-UX packages"

    mk_option \
        OPTION="depot-dir" \
        VAR="MK_PACKAGE_DEPOT_DIR" \
        PARAM="path" \
        DEFAULT="$MK_PACKAGE_DIR/depot" \
        HELP="Subdirectory for built HP-UX packages"
}

configure()
{
    mk_declare -e MK_PACKAGE_DEPOT_DIR
    mk_declare -o DEPOT_CONTROL

    if mk_check_program VAR=SWPACKAGE /usr/sbin/swpackage && [ "$MK_PACKAGE_DEPOT" = "yes" ]
    then
        mk_msg "HP-UX package building: enabled"
        MK_PACKAGE_DEPOT_ENABLED=yes
    else
        mk_msg "HP-UX package building: disabled"
        MK_PACKAGE_DEPOT_ENABLED=no
    fi
}

#<
# @brief Test if HP-UX package building is enabled
# @usage
#
# Returns <lit>0</lit> (logical true) if HP-UX packaging is available
# and was enabled by the user and <lit>1</lit> (logical false)
# otherwise.
#>
mk_depot_enabled()
{
    [ "$MK_PACKAGE_DEPOT_ENABLED" = "yes" ]
}

#<
# @brief Begin HP-UX package definition
# @usage PACKAGE=name PSF=psf
# @option PACKAGE=name Sets the name of the package
# @option VERSION=ver Sets the version of the package
# @option PSF=psf Designates a template product specification
# file to use.
# @option CONTROLFILES=cfiles Specifies a set of control
# files which may be referenced by the product specification
# These will be processed as by <funcref>mk_output_file</funcref>
# and placed in a directory which may be referenced from the
# product specification as <lit>@DEPOT_CONTROL@</lit>.
#
# Begins the definition of an HP-UX package to be built.
# You must provide a template product specification file
# which lays out the structure and metadata of your
# distribution.  The file will be processed as by
# <funcref>mk_output_file</funcref>.  You must specify one
# or more filesets with with <funcref>mk_depot_sub_do</funcref>.
#>
mk_depot_do()
{
    mk_push_vars PACKAGE VERSION PSF CONTROLFILES
    mk_parse_params
    
    DEPOT_DIR=".depot-${PACKAGE}"
    DEPOT_PACKAGE="$PACKAGE"
    DEPOT_VERSION="$VERSION"
    DEPOT_DEPS=""

    mk_resolve_file ".depot-${PACKAGE}"
    DEPOT_DIR="$result"
    mk_safe_rm "$DEPOT_DIR"

    DEPOT_CONTROL="$DEPOT_DIR/control"

    mk_output_file INPUT="$PSF" OUTPUT="@$DEPOT_DIR/product.psf"
    mk_quote "$result"
    DEPOT_DEPS="$DEPOT_DEPS $result"


    mk_unquote_list "$CONTROLFILES"
    for _ctl
    do
        mk_basename "$_ctl"
        _name="${result%.in}"
        mk_output_file INPUT="$_ctl" OUTPUT="@$DEPOT_CONTROL/$_name"
        mk_quote "$result"
        DEPOT_DEPS="$DEPOT_DEPS $result"
    done

    DEPOT_FILESETS=""

    mk_package_targets()
    {
        mk_quote_list "$@"
        DEPOT_DEPS="$DEPOT_DEPS $result"
        
        for _i
        do
            echo "${_i#@$MK_STAGE_DIR}"
        done >> "${DEPOT_FILESET}.files"
    }
    
    mk_package_dirs()
    {
        for _i
        do
            echo "$_i"
        done >> "${DEPOT_FILESET}.dirs"
    }

    mk_pop_vars
}

#<
# @brief Begin HP-UX fileset definition
# @usage SUBPACKAGE=name
# @option SUBPACKAGE=name Sets the name of the fileset
#
# Begins the definition of a fileset.  This will create
# an output variable of the form <var>DEPOT_FILESET_<varname>name</varname></var>
# which can be substituted into the correct place in the
# template PSF file specified to <funcref>mk_depot_do</funcref>.
#
# After invoking this function, you can use functions
# such as <funcref>mk_package_targets</funcref> or
# <funcref>mk_package_patterns</funcref> to add files
# to the fileset.  End the definition with
# <funcref>mk_depot_sub_done</funcref>.
#>
mk_depot_sub_do()
{
    mk_push_vars SUBPACKAGE
    mk_parse_params
    
    DEPOT_SUB="$SUBPACKAGE"
    DEPOT_FILESET="$DEPOT_DIR/$SUBPACKAGE"
    DEPOT_FILESETS="$DEPOT_FILESETS $SUBPACKAGE"
    
    _mk_depot_files_begin
    
    mk_pop_vars
}

#<
# @brief End HP-UX subpackage definition
# @usage
#
# Ends an HP-UX subpackage definition started
# with <funcref>mk_depot_sub_do</funcref>.
#>
mk_depot_sub_done()
{
    _mk_depot_files_end
}
    
#<
# @brief End HP-UX package definition
# @usage
#
# Ends an HP-UX package definition started
# with <funcref>mk_depot_do</funcref>.
#>
mk_depot_done()
{
    _mk_depot_files_end

    mk_target \
        TARGET="@${MK_PACKAGE_DEPOT_DIR}/${DEPOT_PACKAGE}" \
        DEPS="$DEPOT_DEPS" \
        mk_run_script build-depot '$@' "${DEPOT_VERSION}" "@${DEPOT_DIR}" "*$DEPOT_FILESETS"
    master="$result"

    unset -f mk_package_files mk_package_dirs

    mk_add_package_target "$master"

    result="$master"
}

_mk_depot_files_begin()
{
    mk_run_or_fail touch "${DEPOT_FILESET}.files"
    mk_run_or_fail touch "${DEPOT_FILESET}.dirs"
}

_mk_depot_files_end()
{
    :
}
