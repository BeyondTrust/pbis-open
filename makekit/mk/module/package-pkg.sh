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
# package-pkg.sh -- build Solaris packages
#
##

### section configure

DEPENDS="core program package"

option()
{
    mk_option \
        OPTION="package-pkg" \
        VAR="MK_PACKAGE_PKG" \
        PARAM="yes|no" \
        DEFAULT="yes" \
        HELP="Enable building Solaris packages"

    mk_option \
        OPTION="pkg-dir" \
        VAR="MK_PACKAGE_PKG_DIR" \
        PARAM="path" \
        DEFAULT="$MK_PACKAGE_DIR/pkg" \
        HELP="Subdirectory for built Solaris packages"
}

configure()
{
    mk_declare -e MK_PACKAGE_PKG_DIR

    mk_check_program pkgmk
    mk_check_program pkgtrans

    if [ "$MK_PACKAGE_PKG" = "yes" -a "$MK_HOST_OS" = "solaris" -a -n "$PKGMK" -a -n "$PKGTRANS" ]
    then
        mk_msg "Solaris package building: enabled"
        MK_PACKAGE_PKG_ENABLED=yes
    else
        mk_msg "Solaris package building: disabled"
        MK_PACKAGE_PKG_ENABLED=no
    fi
}

#<
# @brief Test if Solaris package building is enabled
# @usage
#
# Returns <lit>0</lit> (logical true) if Solaris packaging is
# available and was enabled by the user and <lit>1</lit> (logical false)
# otherwise.
#>
mk_pkg_enabled()
{
    [ "$MK_PACKAGE_PKG_ENABLED" = "yes" ]
}

_mk_pkg_process_infofiles()
{
    mk_unquote_list "$INFOFILES"
    for _info
    do
        _dest="${PKG_SUBPKGDIR}/info/${_info##*/}"
        _dest="${_dest%.in}"
        mk_output_file INPUT="$_info" OUTPUT="@$_dest"
    done
}

#<
# @brief Begin Solaris package definition
# @usage PACKAGE=name VERSION=ver
# @option PACKAGE=name Sets the name of the package
# @option VERSION=ver Sets the version of the package
#
# Begins the definition of a Solaris .pkg file.
# A .pkg file is a tar-like archive which contains one
# or more actual Solaris packages.  You must specify one
# or more of these "subpackages" with <funcref>mk_pkg_sub_do</funcref>
# and <funcref>mk_pkg_sub_done</funcref>.
#
# Use <funcref>mk_pkg_done</funcref> to complete the definition.
#>
mk_pkg_do()
{
    mk_push_vars PACKAGE VERSION
    mk_parse_params
    
    PKG_PKGDIR=".pkg-${PACKAGE}"
    PKG_PACKAGE="$PACKAGE"
    PKG_VERSION="$VERSION"
    PKG_DEPS=""

    mk_resolve_file "$PKG_PKGDIR"
    PKG_RES_PKGDIR="$result"
    mk_safe_rm "$PKG_RES_PKGDIR"
    mk_mkdir "$PKG_RES_PKGDIR"

    PKG_SUBPKGS=""

    mk_package_targets()
    {
        mk_quote_list "$@"
        PKG_DEPS="$PKG_DEPS $result"
        
        for _i
        do
            echo "t ${_i#@$MK_STAGE_DIR/}"
        done >> "${PKG_MANIFEST}"

        for _i
        do
            _mk_pkg_process_dirs "${_i#@$MK_STAGE_DIR/}"
        done >> "${PKG_MANIFEST}.dirs"
    }
    
    mk_package_dirs()
    {
        for _i in "$@"
        do
            echo "d ${_i#/}"
        done >> "${PKG_MANIFEST}"

        for _i
        do
            echo "!${_i#/}"
            _mk_pkg_process_dirs "${_i#/}"
        done >> "${PKG_MANIFEST}.dirs"
    }

    mk_pop_vars
}

#<
# @brief Begin Solaris subpackage
# @usage SUBPACKAGE=name INFOFILES=infofiles
# @option SUBPACKAGE=name Sets the name of the subpackage.
# Solaris package names must be very short (no more than
# 20 characters), and are typically formed from an abbreviation
# of the vendor's name in uppercase, followed by
# an abbreviation of the product name in lowercase, followed
# by a one- or two-character abbreviation of what the subpackage
# contains. For example: <lit>VENDprodu</lit>, <lit>VENDprodr</lit>,
# for files going in <lit>/usr</lit> and <lit>/</lit> (root),
# respectively.
# @option INFOFILES=infofiles Specifies a list of info files
# that should be included.  The files will be processed as
# by <funcref>mk_output_file</funcref> and have <lit>.in</lit>
# stripped from the end of the name if present.  You must provide
# a <lit>pkginfo</lit> file at a minimum.
#
# Begins the definition of a "real" Solaris package within the
# current .pkg archive being defined.  Use functions such as
# <funcref>mk_package_patterns</funcref> to include files.
#
# By default, all intermediate directories for included files
# will be specified with "don't care" permissions, meaning they
# are expected to already exist on the system.  This is usually
# the desired behavior, e.g. your package should not try to
# override permissions on <lit>/usr/lib</lit>. If you want a
# directory to be owned by the package, specify it with
# <funcref>mk_package_dirs</funcref>.
#
# Typically, subpackages must be used to separate files that
# end up in different mount points, e.g. <lit>/usr</lit> versus
# <lit>/opt</lit> versus <lit>/</lit>.  This prevents conflicts
# when certain filesystems are shared via NFS or Solaris Zones.
#
# Use <funcref>mk_pkg_sub_done</funcref> to complete the definition.
#>
mk_pkg_sub_do()
{
    mk_push_vars SUBPACKAGE INFOFILES
    mk_parse_params
    
    PKG_SUBPACKAGE="$SUBPACKAGE"
    PKG_SUBPKGDIR="$PKG_RES_PKGDIR/subpkg-$PKG_SUBPACKAGE"
    PKG_MANIFEST="$PKG_SUBPKGDIR/manifest"
    PKG_SUBPKGS="$PKG_SUBPKGS $PKG_SUBPACKAGE"
    
    mk_mkdir "$PKG_SUBPKGDIR"
    
    _mk_pkg_process_infofiles
    _mk_pkg_files_begin
    
    mk_pop_vars
}

#<
# @brief End Solaris subpackage
# @usage
#
# Ends the subpackage defininition started with
# <funcref>mk_pkg_sub_do</funcref>.
#>
mk_pkg_sub_done()
{
    _mk_pkg_files_end
    
    unset PKG_SUBPACKAGE PKG_SUBPKGDIR PKG_MANIFEST PKG_INFOFILES
}

#<
# @brief End Solaris package definition
# @usage
#
# Ends a Solaris packages definition started
# with <funcref>mk_pkg_do</funcref>.
#>
mk_pkg_done()
{
    _mk_pkg_files_end

    mk_target \
        TARGET="@${MK_PACKAGE_PKG_DIR}/${PKG_PACKAGE}" \
        DEPS="$PKG_DEPS" \
        _mk_build_pkg \
        '$@' "$PKG_VERSION" "$PKG_RES_PKGDIR" "*$PKG_SUBPKGS"
    master="$result"

    unset PKG_PACKAGE PKG_VERSION PKG_PKGDIR PKG_SUBPKGS
    unset -f mk_package_files mk_package_dirs

    mk_add_package_target "$master"

    result="$master"
}

_mk_pkg_files_begin()
{
    mk_run_or_fail touch "${PKG_MANIFEST}"
    mk_run_or_fail touch "${PKG_MANIFEST}.dirs"
}

_mk_pkg_files_end()
{
    :
}

_mk_pkg_process_dirs()
{
    _IFS="$IFS"
    IFS="/"
    set -- ${1%/*}
    IFS="$_IFS"
    _dir=""
    
    for _j
    do
        _dir="$_dir/$_j"
        echo "${_dir#/}"
    done
}

### section build

_mk_build_pkg()
{
    # $1 = output directory
    # $2 = package version
    # $3 = pkg directory
    # ... = subpackages

    PKG_OUTPUT="$1"
    PKG_VER="$2"
    PKG_DIR="$3"
    PKG_NAME="${PKG_OUTPUT#$MK_PACKAGE_PKG_DIR/}"

    shift 3

    mk_msg_domain "pkg"

    mk_msg "begin $PKG_NAME"

    mk_mkdir "$PKG_OUTPUT"
    
    for PKG_SUBNAME
    do
        PKG_SUBDIR="$PKG_DIR/subpkg-$PKG_SUBNAME"
        
        # Now that all files in the manifest are built, we can
        # generate the actual prototype file
        {
            for _misc in "${PKG_SUBDIR}/info/"*
            do
                [ -e "$_misc" ] && echo "i ${_misc##*/}=${MK_ROOT_DIR}/${_misc}"
            done
            
            echo "!default 0644 root other"
            
            {
                sort < "${PKG_SUBDIR}/manifest.dirs" |
                ${AWK} '/^!.*/ { seen[substr($0,2)] = 1; next; } { if (!seen[$0]) { seen[$0] = 1; printf("d none /%s ? ? ?\n", $0); } }'
            } || mk_fail "could not process intermediate directory list"

            while read -r TYPE FILE
            do
                case "$TYPE" in
                    t)
                        if [ -h "$MK_STAGE_DIR/$FILE" ]
                        then
                            dest="`file -h "$MK_STAGE_DIR/$FILE"`"
                            dest="${dest#*symbolic link to }"
                            echo "s none /$FILE=$dest"
                        elif [ -d "$MK_STAGE_DIR/$FILE" ]
                        then
                            echo "d none /$FILE 0755"
                        else
                            MODE=$(pkgproto "$MK_STAGE_DIR/$FILE" | cut -d" " -f4) || mk_fail "could not find file mode"
                            echo "f none /$FILE=$MK_STAGE_DIR/$FILE $MODE"
                        fi
                        ;;
                    d)
                        echo "d none /$FILE 0755"
                        ;;
                esac
            done < "$PKG_SUBDIR/manifest" || mk_fail "could not read manifest"
        } > "$PKG_SUBDIR/prototype" || mk_fail "could not create prototype"

        mk_msg "pkgmk $PKG_SUBNAME"

        mk_run_quiet_or_fail \
            ${PKGMK} -o \
            -d "$PKG_DIR" \
            -b "${MK_ROOT_DIR}" \
            -f "${PKG_SUBDIR}/prototype" \
            "${PKG_SUBNAME}"

    done

    mk_msg "pkgtrans $PKG_NAME"

    mk_run_quiet_or_fail \
        pkgtrans \
        "${PKG_DIR}" \
        "${PKG_NAME}-${PKG_VER}-${MK_HOST_ARCH}.pkg" \
        "$@"
   
    mk_run_or_fail mv -f "$PKG_DIR/${PKG_NAME}-${PKG_VER}-${MK_HOST_ARCH}.pkg" "$PKG_OUTPUT/"
    
    mk_msg "built ${PKG_NAME}-${PKG_VER}-${MK_HOST_ARCH}.pkg"
    
    mk_msg "end $PKG_NAME"
}
