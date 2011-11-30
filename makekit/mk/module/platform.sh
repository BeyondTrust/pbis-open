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

#<
# @function mk_multiarch_do
# @brief Execute commands for multiple ISAs
# @usage
# 
# Executes a set of commands beginning with <funcref>mk_multiarch_do</funcref>
# and ending with <funcref>mk_multiarch_done</funcref> for each ISA of the
# host system.  During the configure phase, this allows running tests
# or maniuplating variables for each ISA where the results might differ
# between them (e.g. pointer size, system library directory, library file extension).
# During the make phase, it allows building multiple versions of a binary for
# each ISA.  On systems that use "fat" binaries, this function is a no-op
# during the make phase.
#
# @example
# configure()
# {
#     mk_config_header "include/config.h"
#
#     mk_multiarch_do
#         mk_check_sizeofs HEADERDEPS="stdlib.h" "void*" "size_t"
#         mk_define MY_LIB_DIR "\"$MK_LIBDIR/foobar\""
#     mk_multiarch_done
# }
#
# make()
# {
#     mk_multiarch_do
#         mk_library LIB="foobarclient" SOURCES="foobar.c"
#     mk_multiarch_done
# }
# @endexample
#>
alias mk_multiarch_do='for __sys in ${_MK_MULTIARCH_SYS}; do mk_system "$__sys"'

#<
# @function mk_multiarch_done
# @brief End mk_multiarch_do
# @usage
#
# Used to terminate <funcref>mk_multiarch_do</funcref>.
#>
alias mk_multiarch_done='done; mk_system "host"'

#<
# @function mk_compat_do
# @brief Execute commands for alternate ISAs
# @usage
#
# Behaves like <funcref>mk_multiarch_do</funcref>, but skips
# the primary ISA for the host system.
#>
alias mk_compat_do='for __sys in ${_MK_COMPAT_SYS}; do mk_system "$__sys"'

#<
# @function mk_compat_done
# @brief End mk_compat_do
# @usage
#
# Used to terminate <funcref>mk_compat_do</funcref>.
#>
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
    mk_push_vars SYSTEM result
    mk_parse_params

    if [ "$MK_CANONICAL_SYSTEM" = "$SYSTEM" ]
    then
        mk_set "$1" "$2"
    elif [ "$SYSTEM" = "build" ]
    then
        for __isa in ${MK_BUILD_ISAS}
        do
            mk_set_system_var SYSTEM="$SYSTEM/$__isa" "$@"
        done
    elif [ "$SYSTEM" = "host" ]
    then
        for __isa in ${MK_HOST_ISAS}
        do
            mk_set_system_var SYSTEM="$SYSTEM/$__isa" "$@"
        done
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

#<
# @brief Set target system
# @usage sys
# 
# Sets the system which will be the target of subsequent configure tests
# or build products.  This is important for cross-compiling or multiarchitecture
# builds.  The parameter <param>sys</param> may be either "host" to target
# the host system (the system that will run the end product of the build),
# or "build" to target the build system (the system running MakeKit).
#
# Each system may support several ISAs (instruction set architectures)
# which can be targetted individually by suffixing /<var>isa</var> to the
# end of <param>sys</param>, e.g. build/x86_64 or host/ppc64.  This allows
# exact control over which ISA is used for a configure test or a program
# or library.
#
# When targetting "host" or "build" rather than a specific ISA, the behavior
# is more complicated.  During the configure phase, tests will target
# the primary ISA for the system but assume that the result applies to all
# ISAs for the sake of speed.  During the make phase, the behavior varies
# by OS.  On systems that use "fat" or "universal" binaries, a multiarchitecture
# binary targetting all ISAs will be produced.  On other systems, the
# primary ISA will be targetted.
#
# Certain shell variables are considered "system" variables and will have
# their values swapped out appropriately when switching the target system,
# e.g. <var>MK_LIBDIR</var>.  You can declare your own system variables with
# <lit><funcref>mk_declare</funcref> -s</lit>.
#
# This function gives fine-grained control that may not be necessary
# if all you need is multiarchitecture support.  In this case, use
# <funcref>mk_multiarch_do</funcref>.
#>
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

_mk_declare_system()
{
    case " $MK_SYSTEM_VARS " in
        " $1 ")
            return 0
            ;;
    esac
           
    MK_SYSTEM_VARS="$MK_SYSTEM_VARS $1"

    if ${_inherited}
    then
        for __target in ${MK_ALL_SYSTEMS}
        do
            _mk_define_name "$__target"
            _mk_declare_inherited "$1_$result"
        done
    fi

    if ${_exported}
    then
        for __target in ${MK_ALL_SYSTEMS}
        do
            _mk_define_name "$__target"
            _mk_declare_exported "$1_$result"
        done        
    fi
}

mk_declare_system_var()
{
    mk_deprecated "mk_declare_system_var is deprecated; use mk_declare -s instead"

    mk_push_vars EXPORT
    mk_parse_params

    _inherited=true
    _exported=true

    if [ "$EXPORT" = "no" ]
    then
        _inherited=false
        _exported=false
    fi

    for __var
    do
        _mk_declare_system "$__var"
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
        Darwin)
            _default_MK_BUILD_OS="darwin"
            ;;
        AIX)
            _default_MK_BUILD_OS="aix"
            ;;
        HP-UX)
            _default_MK_BUILD_OS="hpux"
            ;;
        *)
            _default_MK_BUILD_OS="unknown"
            ;;
    esac

    case "$_default_MK_BUILD_OS" in
        darwin)
            case `uname -m` in
                i386)
                    if [ "`sysctl -n hw.optional.x86_64 2>/dev/null`" = "1" ]
                    then
                        _default_MK_BUILD_ARCH="x86_64"
                    else
                        _default_MK_BUILD_ARCH="x86"
                    fi
                    ;;
                x86_64)
                    _default_MK_BUILD_ARCH="x86_64"
                    ;;
                *)
                    mk_fail "unknown architecture: `uname -m`"
                    ;;
            esac
            ;;
        solaris)
            _isainfo="`isainfo`"
            case "$_isainfo" in
                "sparcv9"*)
                    _default_MK_BUILD_ARCH="sparcv9"
                    ;;
                "sparc"*)
                    _default_MK_BUILD_ARCH="sparc"
                    ;;
                "amd64"*)
                    _default_MK_BUILD_ARCH="x86_64"
                    ;;
                "i386"*)
                    _default_MK_BUILD_ARCH="x86"
                    ;;
                *)
                    mk_fail "unknown isainfo: $_isainfo"
                    ;;
            esac
            ;;
        aix)
            # AIX only supports POWER, making this easy
            _default_MK_BUILD_ARCH="powerpc"
            ;;
        hpux)
            if hp-pa 2>&1 >/dev/null
            then
                case `/usr/bin/getconf SC_CPU_VERSION 2>/dev/null` in
                    523)
                        _default_MK_BUILD_ARCH="hppa1.0"
                        ;;
                    528)
                        _default_MK_BUILD_ARCH="hppa1.1"
                        ;;
                    532)
                        _default_MK_BUILD_ARCH="hppa2.0"
                        ;;
                    *)
                        _default_MK_BUILD_ARCH="unknown"
                        ;;
                esac
            else
                case `uname -m` in
                    ia64)
                        _default_MK_BUILD_ARCH="ia64"
                        ;;
                    *)
                        mk_fail "unknown architecture: `uname -m`"
                        ;;
                esac
            fi
            ;;
        *)
            case `uname -m` in
                i?86|i86pc)
                    _default_MK_BUILD_ARCH="x86"
                    ;;
                x86_64|amd64)
                    _default_MK_BUILD_ARCH="x86_64"
                    ;;
                ppc)
                    _default_MK_BUILD_ARCH="powerpc"
                    ;;
                *)
                    mk_fail "unknown architecture: `uname -m`"
                    ;;
            esac
            ;;
    esac

    case "${_default_MK_BUILD_OS}-${_default_MK_BUILD_ARCH}" in
        *"-x86")
            _default_MK_BUILD_ISAS="x86_32"
            ;;
        "linux-x86_64"|"darwin-x86_64")
            _default_MK_BUILD_ISAS="x86_64 x86_32"
            ;;
        "solaris-x86_64")
            _default_MK_BUILD_ISAS="x86_32 x86_64"
            ;;
        *"-sparcv9")
            _default_MK_BUILD_ISAS="sparc_32 sparc_64"
            ;;
        *"-sparc")
            _default_MK_BUILD_ISAS="sparc_32"
            ;;
        *"-powerpc")
            _default_MK_BUILD_ISAS="ppc32 ppc64"
            ;;
        *"-hppa1."*)
            _default_MK_BUILD_ISAS="hppa32"
            ;;
        *"-hppa2.0")
            _default_MK_BUILD_ISAS="hppa32 hppa64"
            ;;
        "hpux-ia64")
            _default_MK_BUILD_ISAS="ia64_32 ia64_64"
            ;;
        *)
            _default_MK_BUILD_ISAS="$_default_MK_BUILD_ARCH"
            ;;
    esac

    case "$_default_MK_BUILD_OS" in
        linux)
            if type lsb_release >/dev/null 2>&1
            then
                case "`lsb_release -si`" in
                    "SUSE LINUX")
                        # Have to determine opensuse/etc. from description
                        # To make it even more fun, the description contains
                        # extraneous double quotes
                        _default_MK_BUILD_DISTRO="`lsb_release -sd | sed 's/\"//g' | awk '{print $1;}' | tr 'A-Z' 'a-z'`"
                        ;;
                    "RedHatEnterprise"*)
                        _default_MK_BUILD_DISTRO="rhel"
                        ;;           
                    "FedoraCore")
                        _default_MK_BUILD_DISTRO="fedora"
                        ;;
                    *)
                        _default_MK_BUILD_DISTRO="`lsb_release -si | awk '{print $1;}' | tr 'A-Z' 'a-z'`"
                        ;;
                esac
                _default_MK_BUILD_DISTRO_VERSION="`lsb_release -sr | tr 'A-Z' 'a-z'`"
            elif [ -f /etc/redhat-release ]
            then
                case "`cat /etc/redhat-release`" in
                    "Fedora"*)
                        _default_MK_BUILD_DISTRO="fedora"
                        _default_MK_BUILD_DISTRO_VERSION="`cat /etc/redhat-release | awk '{print $3;}'`"
                        ;;
                    "Red Hat Enterprise Linux"*)
                        _default_MK_BUILD_DISTRO="rhel"
                        _default_MK_BUILD_DISTRO_VERSION="`cat /etc/redhat-release | awk '{print $7;}'`"
                        ;;
                    *)
                        _default_MK_BUILD_DISTRO="redhat"
                        _default_MK_BUILD_DISTRO_VERSION="unknown"
                        ;;
                esac
            else
                _default_MK_BUILD_DISTRO="unknown"
                _default_MK_BUILD_DISTRO_VERSION="unknown"
            fi
            _default_MK_BUILD_MULTIARCH="separate"
            ;;
        freebsd)
            __release="`uname -r`"
            _default_MK_BUILD_DISTRO="`uname -s | tr 'A-Z' 'a-z'`"
            _default_MK_BUILD_DISTRO_VERSION="${__release%%-*}"
            _default_MK_BUILD_MULTIARCH="separate"
            ;;
        solaris)
            __release="`uname -r`"
            _default_MK_BUILD_DISTRO="solaris"
            _default_MK_BUILD_DISTRO_VERSION="${__release#*.}"
            _default_MK_BUILD_MULTIARCH="separate"
            ;;
        darwin)
            case "`sw_vers -productName 2>/dev/null`" in
                "Mac OS X")
                    _default_MK_BUILD_DISTRO="macosx"
                    _default_MK_BUILD_DISTRO_VERSION="`sw_vers -productVersion`"
                    ;;
                *)
                    _default_MK_BUILD_DISTRO="unknown"
                    _default_MK_BUILD_DISTRO_VERSION="unknown"
                    ;;
            esac
            _default_MK_BUILD_MULTIARCH="combine"
            ;;
        aix)
            _default_MK_BUILD_DISTRO="aix"
            _default_MK_BUILD_DISTRO_VERSION="`uname -v`.`uname -r`"
            _default_MK_BUILD_MULTIARCH="separate"
            ;;
        hpux)
            _default_MK_BUILD_DISTRO="hpux"
            __release="`uname -r`"
            _default_MK_BUILD_DISTRO_VERSION="${__release#B.}"
            _default_MK_BUILD_MULTIARCH="separate"
            ;;
        *)
            _default_MK_BUILD_DISTRO="unknown"
            _default_MK_BUILD_DISTRO_VERSION="unknown"
            _default_MK_BUILD_MULTIARCH="none"
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
        OPTION=build-multiarch \
        VAR=MK_BUILD_MULTIARCH \
        DEFAULT="$_default_MK_BUILD_MULTIARCH" \
        HELP="Build multiarchitecture binary style"

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

    case "$MK_BUILD_DISTRO" in
        centos|redhat|fedora|rhel)
            _distro_archetype="redhat"
            ;;
        debian|ubuntu)
            _distro_archetype="debian"
            ;;
        suse|opensuse)
            _distro_archetype="suse"
            ;;
        *)
            _distro_archetype="$MK_BUILD_DISTRO"
            ;;
    esac

    mk_option \
        OPTION=build-distro-archetype \
        VAR=MK_BUILD_DISTRO_ARCHETYPE \
        DEFAULT="$_distro_archetype" \
        HELP="Build operating system distribution archetype"

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
        OPTION=host-multiarch \
        VAR=MK_HOST_MULTIARCH \
        DEFAULT="$MK_BUILD_MULTIARCH" \
        HELP="Host multiarchitecture binary style"

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

    case "$MK_HOST_DISTRO" in
        centos|redhat|fedora|rhel)
            _distro_archetype="redhat"
            ;;
        debian|ubuntu)
            _distro_archetype="debian"
            ;;
        suse|opensuse)
            _distro_archetype="suse"
            ;;
        *)
            _distro_archetype="$MK_HOST_DISTRO"
            ;;
    esac

    mk_option \
        OPTION=host-distro-archetype \
        VAR=MK_HOST_DISTRO_ARCHETYPE \
        DEFAULT="$_distro_archetype" \
        HELP="Host operating system distribution archetype"

    MK_BUILD_PRIMARY_ISA="${MK_BUILD_ISAS%% *}"
    MK_HOST_PRIMARY_ISA="${MK_HOST_ISAS%% *}"
}

_mk_platform_set_ext()
{
    case "$1:${2#*/}" in
        darwin:*)
            mk_set_system_var SYSTEM="$2" MK_LIB_EXT ".dylib"
            mk_set_system_var SYSTEM="$2" MK_DLO_EXT ".so"
            ;;
        hpux:hppa*)
            mk_set_system_var SYSTEM="$2" MK_LIB_EXT ".sl"
            mk_set_system_var SYSTEM="$2" MK_DLO_EXT ".sl"
            ;;
        *)
            mk_set_system_var SYSTEM="$2" MK_LIB_EXT ".so"
            mk_set_system_var SYSTEM="$2" MK_DLO_EXT ".so"
            ;;
    esac   
}

configure()
{
    for _isa in ${MK_BUILD_ISAS}
    do
        MK_ALL_SYSTEMS="$MK_ALL_SYSTEMS build/$_isa"
    done

    for _isa in ${MK_HOST_ISAS}
    do
        MK_ALL_SYSTEMS="$MK_ALL_SYSTEMS host/$_isa"
    done

    mk_declare -e \
        MK_BUILD_OS MK_BUILD_DISTRO MK_BUILD_DISTRO_VERSION \
        MK_BUILD_DISTRO_ARCHETYPE MK_BUILD_ARCH MK_BUILD_ISAS \
        MK_BUILD_PRIMARY_ISA MK_HOST_OS MK_HOST_DISTRO \
        MK_HOST_DISTRO_VERSION MK_HOST_DISTRO_ARCHETYPE \
        MK_HOST_ARCH MK_HOST_ISAS MK_HOST_PRIMARY_ISA \
        MK_SYSTEM_VARS MK_HOST_MULTIARCH MK_BUILD_MULTIARCH \
        MK_ALL_SYSTEMS

    mk_declare -s -e \
        MK_OS MK_DISTRO MK_DISTRO_VERSION MK_DISTRO_ARCHETYPE \
        MK_ARCH MK_ISAS MK_MULTIARCH MK_ISA MK_DLO_EXT MK_LIB_EXT

    for _isa in ${MK_BUILD_ISAS}
    do
        mk_set_system_var SYSTEM="build/$_isa" MK_OS "$MK_BUILD_OS"
        mk_set_system_var SYSTEM="build/$_isa" MK_DISTRO "$MK_BUILD_DISTRO"
        mk_set_system_var SYSTEM="build/$_isa" MK_DISTRO_VERSION "$MK_BUILD_DISTRO_VERSION"
        mk_set_system_var SYSTEM="build/$_isa" MK_DISTRO_ARCHETYPE "$MK_BUILD_DISTRO_ARCHETYPE"
        mk_set_system_var SYSTEM="build/$_isa" MK_ARCH "$MK_BUILD_ARCH"
        mk_set_system_var SYSTEM="build/$_isa" MK_ISAS "$MK_BUILD_ISAS"
        mk_set_system_var SYSTEM="build/$_isa" MK_MULTIARCH "$MK_BUILD_MULTIARCH"
        mk_set_system_var SYSTEM="build/$_isa" MK_ISA "$_isa"
        _mk_platform_set_ext "$MK_BUILD_OS" "build/$_isa"
    done

    for _isa in ${MK_HOST_ISAS}
    do
        mk_set_system_var SYSTEM="host/$_isa" MK_OS "$MK_HOST_OS"
        mk_set_system_var SYSTEM="host/$_isa" MK_DISTRO "$MK_HOST_DISTRO"
        mk_set_system_var SYSTEM="host/$_isa" MK_DISTRO_VERSION "$MK_HOST_DISTRO_VERSION"
        mk_set_system_var SYSTEM="host/$_isa" MK_DISTRO_ARCHETYPE "$MK_HOST_DISTRO_ARCHETYPE"
        mk_set_system_var SYSTEM="host/$_isa" MK_ARCH "$MK_HOST_ARCH"
        mk_set_system_var SYSTEM="host/$_isa" MK_MULTIARCH "$MK_HOST_MULTIARCH"
        mk_set_system_var SYSTEM="host/$_isa" MK_ISAS "$MK_HOST_ISAS"
        mk_set_system_var SYSTEM="host/$_isa" MK_ISA "$_isa"
        _mk_platform_set_ext "$MK_HOST_OS" "host/$_isa"
    done

    for _sys in build host
    do
        mk_system "$_sys"

        mk_msg "$_sys operating system: $MK_OS"
        mk_msg "$_sys distribution: $MK_DISTRO"
        mk_msg "$_sys distribution version: $MK_DISTRO_VERSION"
        mk_msg "$_sys processor architecture: $MK_ARCH"
        mk_msg "$_sys instruction set architectures: $MK_ISAS"
        mk_msg "$_sys multiarchitecture binary style: $MK_MULTIARCH"
    done

    if [ "$MK_BUILD_OS:$MK_BUILD_ARCH" != "$MK_HOST_OS:$MK_HOST_ARCH" ]
    then
        MK_CROSS_COMPILING="yes"
    else
        MK_CROSS_COMPILING="no"
    fi

    _MK_MULTIARCH_SYS_CONFIGURE=
    _MK_MULTIARCH_SYS_MAKE=

    case "$MK_HOST_MULTIARCH" in
        separate)
            for _isa in ${MK_HOST_ISAS}
            do
                _MK_MULTIARCH_SYS_CONFIGURE="$_MK_MULTIARCH_SYS_CONFIGURE host/$_isa"
                _MK_MULTIARCH_SYS_MAKE="$_MK_MULTIARCH_SYS_MAKE host/$_isa"
            done
            ;;
        combine)
            for _isa in ${MK_HOST_ISAS}
            do
                _MK_MULTIARCH_SYS_CONFIGURE="$_MK_MULTIARCH_SYS_CONFIGURE host/$_isa"
            done

            _MK_MULTIARCH_SYS_MAKE="host"
            ;;
        none)
            _MK_MULTIARCH_SYS_CONFIGURE="host"
            _MK_MULTIARCH_SYS_MAKE="host"
            ;;
    esac

    set -- ${_MK_MULTIARCH_SYS_CONFIGURE}; shift
    _MK_MULTIARCH_COMPAT_CONFIGURE="$*"

    set -- ${_MK_MULTIARCH_SYS_MAKE}; shift
    _MK_MULTIARCH_COMPAT_MAKE="$*"

    mk_declare -e MK_CROSS_COMPILING

    mk_msg "cross compiling: $MK_CROSS_COMPILING"

    # Register hooks that set the target system to the default
    # or restore any modified system variables at the start of
    # all configure() and make() functions
    mk_add_configure_prehook _mk_platform_restore_system_vars
    mk_add_make_prehook _mk_platform_restore_system_vars

    # Register hooks to set up mk_multiarch_do/mk_compat_do
    # at the beginning of configure and make stages
    mk_add_configure_prehook _mk_platform_configure_multiarch
    mk_add_make_prehook _mk_platform_make_multiarch

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

_mk_platform_configure_multiarch()
{
    _MK_MULTIARCH_SYS="${_MK_MULTIARCH_SYS_CONFIGURE}"
    _MK_COMPAT_SYS="${_MK_MULTIARCH_COMPAT_CONFIGURE}"
}

_mk_platform_make_multiarch()
{
    _MK_MULTIARCH_SYS="${_MK_MULTIARCH_SYS_MAKE}"
    _MK_COMPAT_SYS="${_MK_MULTIARCH_COMPAT_MAKE}"
}
