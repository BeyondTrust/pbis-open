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
# platform.sh -- platform detection and cross-building support
#
##

### section common

alias mk_multiarch_do='for __isa in ${MK_HOST_ISAS}; do mk_system "host/$__isa"'
alias mk_multiarch_done='done; mk_system "host"'
alias mk_compat_do='for __isa in ${MK_HOST_ISAS#$MK_HOST_PRIMARY_ISA}; do mk_system "host/$__isa"'
alias mk_compat_done='done; mk_system "host"'

mk_get_system_var()
{
    mk_push_vars SYSTEM
    mk_parse_params
    
    [ -z "$SYSTEM" ] && SYSTEM="$2"

    if [ "$MK_SYSTEM" = "$SYSTEM" ]
    then
        mk_get "$1"
    else
        _mk_define_name "${1}_$SYSTEM"
        mk_get "$result"
    fi

    mk_pop_vars
}

mk_set_system_var()
{
    mk_push_vars SYSTEM
    mk_parse_params

    if [ "$MK_CANONICAL_SYSTEM" = "$SYSTEM" ]
    then
        mk_set "$1" "$2"
    else
        _mk_define_name "${1}_$SYSTEM"
        mk_set "$result" "$2"
    fi

    mk_pop_vars
}

mk_set_all_isas()
{
        if [ "$MK_SYSTEM" = "${MK_SYSTEM%/*}" ]
    then 
        for __isa in ${MK_ISAS}
        do
                mk_set_system_var SYSTEM="$MK_SYSTEM/$__isa" "$@"
        done
        else
        mk_set "$@"
    fi
}

mk_system()
{
    mk_push_vars suffix canon var
    mk_canonical_system "$1"
    canon="$result"
    
    MK_SYSTEM="$1"
    
    # If we are changing the current system
    if [ "$MK_CANONICAL_SYSTEM" != "$canon" ]
    then
        if [ -n "$MK_CANONICAL_SYSTEM" ]
        then
            # Save all current variable values
            _mk_define_name "$MK_CANONICAL_SYSTEM"
            suffix="$result"
            for var in ${MK_SYSTEM_VARS}
            do
                eval "${var}_${suffix}=\"\$$var\""
            done
        fi

        # Switch to the new system
        MK_CANONICAL_SYSTEM="$canon"

        # Restore variable values
        _mk_define_name "$MK_CANONICAL_SYSTEM"
        suffix="$result"
        for var in ${MK_SYSTEM_VARS}
        do
            eval "${var}=\"\$${var}_${suffix}\""
        done
    fi

    mk_pop_vars
}

mk_canonical_system()
{
    case "$1" in
        ""|host)
            result="host/${MK_HOST_PRIMARY_ISA}"
            ;;
        build)
            result="build/${MK_BUILD_PRIMARY_ISA}"
            ;;
        *)
            result="$1"
    esac
}

mk_run_with_extended_library_path()
{
    unset __env
    
    case "$MK_BUILD_OS" in
        linux|*)
            __env="LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$1"
            ;;
    esac
    
    shift
    env "$__env" "$@"
}

### section configure

mk_declare_system_var()
{
    mk_push_vars EXPORT
    mk_parse_params

    for __var in "$@"
    do
        if ! _mk_contains "$__var" ${MK_SYSTEM_VARS}
        then
            MK_SYSTEM_VARS="$MK_SYSTEM_VARS $__var"
            if [ "$EXPORT" != "no" ]
            then
                for __isa in ${MK_HOST_ISAS}
                do
                    _mk_define_name "host/${__isa}"
                    mk_export "${__var}_$result"
                done
                
                for __isa in ${MK_BUILD_ISAS}
                do
                    _mk_define_name "build/${__isa}"
                    mk_export "${__var}_$result"
                done
            fi
        fi
    done

    mk_pop_vars
}

option()
{
    case `uname` in
        Linux)
            _default_MK_BUILD_OS="linux"
            ;;
        SunOS)
            _default_MK_BUILD_OS="solaris"
            ;;
        FreeBSD)
            _default_MK_BUILD_OS="freebsd"
            ;;
        *)
            _default_MK_BUILD_OS="unknown"
            ;;
    esac

    case `uname -m` in
        i?86|i86pc)
            _default_MK_BUILD_ARCH="x86"
            ;;
        x86_64|amd64)
            _default_MK_BUILD_ARCH="x86_64"
            ;;
        *)
            mk_fail "unknown architecture: `uname -m`"
            ;;
    esac

    case "${_default_MK_BUILD_OS}-${_default_MK_BUILD_ARCH}" in
        *"-x86")
            _default_MK_BUILD_ISAS="x86_32"
            ;;
        "linux-x86_64")
            _default_MK_BUILD_ISAS="x86_64 x86_32"
            ;;
        *)
            _default_MK_BUILD_ISAS="$_default_MK_BUILD_ARCH"
            ;;
    esac

    case "$_default_MK_BUILD_OS" in
        linux)
            if mk_safe_source "/etc/lsb-release"
            then
                case "$DISTRIB_ID" in
                    *)
                        _default_MK_BUILD_DISTRO="`echo "$DISTRIB_ID" | tr 'A-Z' 'a-z'`"
                        _default_MK_BUILD_DISTRO_VERSION="$DISTRIB_RELEASE"
                        ;;
                esac
            else
                _default_MK_BUILD_DISTRO="unknown"
                _default_MK_BUILD_DISTRO_VERSION="unknown"
            fi                
            ;;
        freebsd)
            __release="`uname -r`"
            _default_MK_BUILD_DISTRO="`uname -s | tr 'A-Z' 'a-z'`"
            _default_MK_BUILD_DISTRO_VERSION="${__release%-*}"
            ;;
        solaris)
            __release="`uname -r`"
            _default_MK_BUILD_DISTRO="solaris"
            _default_MK_BUILD_DISTRO_VERSION="${__release#*.}"
            ;;
        *)
            _default_MK_BUILD_DISTRO="unknown"
            _default_MK_BUILD_DISTRO_VERSION="unknown"
            ;;
    esac

    mk_option \
        OPTION=build-os \
        VAR=MK_BUILD_OS \
        DEFAULT="$_default_MK_BUILD_OS" \
        HELP="Build operating system"

    mk_option \
        OPTION=build-arch \
        VAR=MK_BUILD_ARCH \
        DEFAULT="$_default_MK_BUILD_ARCH" \
        HELP="Build CPU architecture"

    mk_option \
        OPTION=build-isas \
        VAR=MK_BUILD_ISAS \
        DEFAULT="$_default_MK_BUILD_ISAS" \
        HELP="Build instruction set architectures"

    mk_option \
        OPTION=build-distro \
        VAR=MK_BUILD_DISTRO \
        DEFAULT="$_default_MK_BUILD_DISTRO" \
        HELP="Build operating system distribution"

    mk_option \
        OPTION=build-distro-version \
        VAR=MK_BUILD_DISTRO_VERSION \
        DEFAULT="$_default_MK_BUILD_DISTRO_VERSION" \
        HELP="Build operating system distribution version"

    mk_option \
        OPTION=host-os \
        VAR=MK_HOST_OS \
        DEFAULT="$MK_BUILD_OS" \
        HELP="Host operating system"

    mk_option \
        OPTION=host-arch \
        VAR=MK_HOST_ARCH \
        DEFAULT="$MK_BUILD_ARCH" \
        HELP="Host CPU architecture"

    mk_option \
        OPTION=host-isas \
        VAR=MK_HOST_ISAS \
        DEFAULT="$MK_BUILD_ISAS" \
        HELP="Host instruction set architectures"

    mk_option \
        OPTION=host-distro \
        VAR=MK_HOST_DISTRO \
        DEFAULT="$MK_BUILD_DISTRO" \
        HELP="Host operating system distribution"

    mk_option \
        OPTION=host-distro-version \
        VAR=MK_HOST_DISTRO_VERSION \
        DEFAULT="$MK_BUILD_DISTRO_VERSION" \
        HELP="Host operating system distribution version"

    MK_BUILD_PRIMARY_ISA="${MK_BUILD_ISAS%% *}"
    MK_HOST_PRIMARY_ISA="${MK_HOST_ISAS%% *}"
}

configure()
{
    mk_export \
        MK_BUILD_OS MK_BUILD_DISTRO MK_BUILD_DISTRO_VERSION \
        MK_BUILD_ARCH MK_BUILD_ISAS MK_BUILD_PRIMARY_ISA \
        MK_HOST_OS MK_HOST_DISTRO MK_HOST_DISTRO_VERSION \
        MK_HOST_ARCH MK_HOST_ISAS MK_HOST_PRIMARY_ISA \
        MK_SYSTEM_VARS

    mk_declare_system_var \
        MK_OS MK_DISTRO MK_DISTRO_VERSION MK_ARCH MK_ISAS MK_ISA \
        MK_DLO_EXT MK_LIB_EXT

    for _isa in ${MK_BUILD_ISAS}
    do
        mk_set_system_var SYSTEM="build/$_isa" MK_OS "$MK_BUILD_OS"
        mk_set_system_var SYSTEM="build/$_isa" MK_DISTRO "$MK_BUILD_DISTRO"
        mk_set_system_var SYSTEM="build/$_isa" MK_DISTRO_VERSION "$MK_BUILD_DISTRO_VERSION"
        mk_set_system_var SYSTEM="build/$_isa" MK_ARCH "$MK_BUILD_ARCH"
        mk_set_system_var SYSTEM="build/$_isa" MK_ISAS "$MK_BUILD_ISAS"
        mk_set_system_var SYSTEM="build/$_isa" MK_ISA "$_isa"
    done

    for _isa in ${MK_HOST_ISAS}
    do
        mk_set_system_var SYSTEM="host/$_isa" MK_OS "$MK_HOST_OS"
        mk_set_system_var SYSTEM="host/$_isa" MK_DISTRO "$MK_HOST_DISTRO"
        mk_set_system_var SYSTEM="host/$_isa" MK_DISTRO_VERSION "$MK_HOST_DISTRO_VERSION"
        mk_set_system_var SYSTEM="host/$_isa" MK_ARCH "$MK_HOST_ARCH"
        mk_set_system_var SYSTEM="host/$_isa" MK_ISAS "$MK_HOST_ISAS"
        mk_set_system_var SYSTEM="host/$_isa" MK_ISA "$_isa"
    done

    for _sys in build host
    do
        mk_system "$_sys"
        
        for _isa in $MK_ISAS
        do
            case "$MK_OS-$_isa" in
                linux-*|solaris-*|freebsd-*)
                    mk_set_system_var SYSTEM="$_sys/$_isa" MK_LIB_EXT ".so"
                    mk_set_system_var SYSTEM="$_sys/$_isa" MK_DLO_EXT ".so"
                    ;;
            esac
        done
        
        mk_msg "$_sys operating system: $MK_OS"
        mk_msg "$_sys distribution: $MK_DISTRO"
        mk_msg "$_sys distribution version: $MK_DISTRO_VERSION"
        mk_msg "$_sys processor architecture: $MK_ARCH"
        mk_msg "$_sys instruction set architectures: $MK_ISAS"
    done

    # Register hooks that set the target system to the default
    # or restore any modified system variables at the start of
    # all configure() and make() functions
    mk_add_configure_prehook _mk_platform_restore_system_vars
    mk_add_make_prehook _mk_platform_restore_system_vars

    # Register hooks that commit all system variables
    # at the end of all configure() and make() functions so that they
    # get written out as exports and restored correctly
    mk_add_configure_posthook _mk_platform_commit_system_vars
    mk_add_make_posthook _mk_platform_commit_system_vars

    # Set the default system now
    mk_system "host"
}

_mk_platform_restore_system_vars()
{
    # Switch system back to default
    MK_SYSTEM="host"
    MK_CANONICAL_SYSTEM="host/${MK_HOST_PRIMARY_ISA}"
    # Restore all variables
    _mk_define_name "$MK_CANONICAL_SYSTEM"
    for ___var in ${MK_SYSTEM_VARS}
    do
        eval "${___var}=\"\$${___var}_${result}\""
    done
}

_mk_platform_commit_system_vars()
{
    if [ -n "$MK_CANONICAL_SYSTEM" ]
    then
        _mk_define_name "$MK_CANONICAL_SYSTEM"
        for ___var in ${MK_SYSTEM_VARS}
        do
            eval "${___var}_${result}=\"\$$___var\""
        done
    fi
}
