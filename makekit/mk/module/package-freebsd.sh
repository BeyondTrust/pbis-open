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
# package-freebsd.sh -- build FreeBSD packages
#
##

### section configure

DEPENDS="core program package"

option()
{
    mk_option \
        OPTION="package-freebsd" \
        VAR="MK_PACKAGE_FREEBSD" \
        PARAM="yes|no" \
        DEFAULT="yes" \
        HELP="Enable building FreeBSD packages"

    mk_option \
        OPTION="freebsd-dir" \
        VAR="MK_PACKAGE_FREEBSD_DIR" \
        PARAM="path" \
        DEFAULT="$MK_PACKAGE_DIR/freebsd" \
        HELP="Subdirectory for built FreeBSD packages"
}

configure()
{
    mk_declare -e MK_PACKAGE_FREEBSD_DIR

    if [ "$MK_PACKAGE_FREEBSD" = "yes" ] &&
       [ "$MK_HOST_OS" = "freebsd" ] &&
       mk_check_program PROGRAM=pkg_create
    then
        mk_msg "FreeBSD package building: enabled"
        MK_PACKAGE_FREEBSD_ENABLED=yes
    else
        mk_msg "FreeBSD package building: disabled"
        MK_PACKAGE_FREEBSD_ENABLED=no
    fi
}

#<
# @brief Test if FreeBSD package building is enabled
# @usage
#
# Returns <lit>0</lit> (logical true) if FreeBSD packaging is
# available and was enabled by the user and <lit>1</lit> (logical false)
# otherwise.
#>
mk_freebsd_enabled()
{
    [ "$MK_PACKAGE_FREEBSD_ENABLED" = "yes" ]
}

#<
# @brief Begin FreeBSD package definition
# @usage PACKAGE=name VERSION=ver SHORT=shortdesc LONG=longdesc
# @option PACKAGE=name Sets the name of the package.
# @option VERSION=ver Sets the version of the package.
# @option SHORT=shortdesc Sets the short description.  If
# <param>shortdesc</param> begins with a <lit>-</lit>, the
# rest of the string is treated as the description.  Otherwise,
# <param>shortdesc</param> is treated as a file which will first
# be processed as by <funcref>mk_output_file</funcref> to yield
# the final short description.
# @option LONG=longdesc Sets the long description.  If
# <param>longdesc</param> begins with a <lit>-</lit>, the
# rest of the string is treated as the description.  Otherwise,
# <param>longdesc</param> is treated as a file which will first
# be processed as by <funcref>mk_output_file</funcref> to yield
# the final long description.
# @option PACKING=file Designates a template packing list
# file to use.  The file needs to specify the <lit>@name</lit>
# directive at a minimum.  If this option is not specified, one
# will be generated automatically.  The file will be processed
# as by <funcref>mk_output_file</funcref>.
# @option PREINST=preinst Specifies a script to run before the
# package is installed.  The file will be processed as by
# <funcref>mk_output_file</funcref>.
# @option POSTINST=postinst Specifies a script to run after the
# package is installed.  The file will be processed as by
# <funcref>mk_output_file</funcref>.
# @option PRERM=prerm Specifies a script to run before the
# package is uninstalled.  The file will be processed as by
# <funcref>mk_output_file</funcref>.
# @option POSTRM=postrm Specifies a script to run after the
# package is uninstalled.  The file will be processed as by
# <funcref>mk_output_file</funcref>.
#
# Begins the definition of a FreeBSD package to be built.
#
# After invoking this function, you can use functions
# such as <funcref>mk_package_targets</funcref> or
# <funcref>mk_package_patterns</funcref> to add files
# to the package.  End the definition of the
# package with <funcref>mk_freebsd_done</funcref>.
#>
mk_freebsd_do()
{
    mk_push_vars PACKAGE VERSION PACKING SHORT LONG PREINST POSTINST PRERM POSTRM
    mk_parse_params
    
    FREEBSD_PKGDIR=".freebsd-${PACKAGE}"
    FREEBSD_PACKAGE="$PACKAGE"
    FREEBSD_VERSION="$VERSION"
    FREEBSD_DEPS=""

    mk_resolve_file "$FREEBSD_PKGDIR"
    FREEBSD_RES_PKGDIR="$result"
    mk_safe_rm "$FREEBSD_RES_PKGDIR"
    mk_mkdir "$FREEBSD_RES_PKGDIR"

    FREEBSD_PACKING="$FREEBSD_RES_PKGDIR/packing"
    FREEBSD_SHORT="$FREEBSD_RES_PKGDIR/short"
    FREEBSD_LONG="$FREEBSD_RES_PKGDIR/long"
    FREEBSD_PREINST="$FREEBSD_RES_PKGDIR/preinst"
    FREEBSD_POSTINST="$FREEBSD_RES_PKGDIR/postinst"
    FREEBSD_PRERM="$FREEBSD_RES_PKGDIR/prerm"
    FREEBSD_POSTRM="$FREEBSD_RES_PKGDIR/postrm"

    if [ -n "$PACKING" ]
    then
        mk_output_file INPUT="$PACKING" OUTPUT="@$FREEBSD_PACKING"
        mk_quote "$PACKING"
        FREEBSD_DEPS="$FREEBSD_DEPS $result"
    else
        mk_run_or_fail echo "@name $PACKAGE-$VERSION" > "$FREEBSD_PACKING"
    fi

    case "$SHORT" in
        "")
            mk_fail "SHORT not specified"
            ;;
        -*)
            mk_run_or_fail echo "${SHORT#-}" > "$FREEBSD_SHORT"
            ;;
        *)
            mk_output_file INPUT="$SHORT" OUTPUT="@$FREEBSD_SHORT"
            mk_quote "$SHORT"
            FREEBSD_DEPS="$FREEBSD_DEPS $result"
            ;;
    esac

    case "$LONG" in
        "")
            mk_fail "LONG not specified"
            ;;
        -*)
            mk_run_or_fail echo "${LONG#-}" > "$FREEBSD_LONG"
            ;;
        *)
            mk_output_file INPUT="$LONG" OUTPUT="@$FREEBSD_LONG"
            mk_quote "$LONG"
            FREEBSD_DEPS="$FREEBSD_DEPS $result"
            ;;
    esac

    if [ -n "$PREINST" ]
    then
        mk_output_file INPUT="$PREINST" OUTPUT="@$FREEBSD_PREINST"
        mk_quote "$PREINST"
        FREEBSD_DEPS="$FREEBSD_DEPS $result"
    else
        mk_run_or_fail printf "" > "$FREEBSD_PREINST"
    fi        

    if [ -n "$POSTINST" ]
    then
        mk_output_file INPUT="$POSTINST" OUTPUT="@$FREEBSD_POSTINST"
        mk_quote "$POSTINST"
        FREEBSD_DEPS="$FREEBSD_DEPS $result"
    else
        mk_run_or_fail printf "" > "$FREEBSD_POSTINST"
    fi

    if [ -n "$PRERM" ]
    then
        mk_output_file INPUT="$PRERM" OUTPUT="@$FREEBSD_PRERM"
        mk_quote "$PRERM"
        FREEBSD_DEPS="$FREEBSD_DEPS $result"
    else
        mk_run_or_fail printf "" > "$FREEBSD_PRERM"
    fi        

    if [ -n "$POSTRM" ]
    then
        mk_output_file INPUT="$POSTRM" OUTPUT="@$FREEBSD_POSTRM"
        mk_quote "$POSTRM"
        FREEBSD_DEPS="$FREEBSD_DEPS $result"
    else
        mk_run_or_fail printf "" > "$FREEBSD_POSTRM"
    fi

    _mk_freebsd_files_begin

    mk_package_targets()
    {
        mk_quote_list "$@"
        FREEBSD_DEPS="$FREEBSD_DEPS $result"
        
        for _i
        do
            echo "${_i#@$MK_STAGE_DIR/}"
        done >> "${FREEBSD_PACKING}"
    }
    
    mk_package_dirs()
    {
        for _i in "$@"
        do
            mk_quote "$_i"
            echo "@exec mkdir -p $result"
        done >> "${FREEBSD_PACKING}"
    }

    mk_pop_vars
}

#<
# @brief End FreeBSD package definition
# @usage
#
# Ends a FreeBSD packages definition started
# with <funcref>mk_freebsd_do</funcref>.
#>
mk_freebsd_done()
{
    _mk_freebsd_files_end

    mk_target \
        TARGET="@${MK_PACKAGE_FREEBSD_DIR}/${FREEBSD_PACKAGE}" \
        DEPS="$FREEBSD_DEPS" \
        _mk_build_freebsd \
        "$FREEBSD_PACKAGE" "$FREEBSD_VERSION" "&$FREEBSD_PKGDIR"
    master="$result"

    unset \
        FREEBSD_PACKAGE FREEBSD_VERSION FREEBSD_PACKING \
        FREEBSD_PREINST FREEBSD_POSTINST \
        FREEBSD_PRERM FREEBSD_POSTRM \
        FREEBSD_PKGDIR
    unset -f mk_package_files mk_package_dirs

    mk_add_package_target "$master"

    result="$master"
}

_mk_freebsd_files_begin()
{
    {
        echo "@cwd /"
        echo "@srcdir ${MK_ROOT_DIR}/${MK_STAGE_DIR}"
        echo "@owner root"
        echo "@group wheel"
    } >> "${FREEBSD_PACKING}"
}

_mk_freebsd_files_end()
{
    :
}

### section build

_mk_build_freebsd()
{
    # $1 = package name
    # $2 = package version
    # $3 = build directory

    mk_msg_domain "freebsd"

    mk_safe_rm "${MK_PACKAGE_FREEBSD_DIR}/$1"
    mk_mkdir "${MK_PACKAGE_FREEBSD_DIR}/$1"
    mk_msg "begin $1"
    
    mk_run_quiet_or_fail ${PKG_CREATE} \
        -f "$3/packing" \
        -c "$3/short" \
        -d "$3/long" \
        -i "$3/preinst" \
        -I "$3/postinst" \
        -k "$3/prerm" \
        -K "$3/postrm" \
        "${MK_PACKAGE_FREEBSD_DIR}/$1/$1-$2-$MK_HOST_ARCH"

    for i in "${MK_PACKAGE_FREEBSD_DIR}/$1"/*.tbz
    do
        mk_msg "built ${i##*/}"
    done
    mk_msg "end $1"
}
