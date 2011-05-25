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
# autotools.sh -- allows building of autotools-based subprojects
#
##

DEPENDS="core path compiler platform"

### section configure

_mk_at_system_string()
{
    mk_get "MK_${1}_OS"

    case "${result}" in
        linux)
            __os="linux-gnu"
            ;;
        freebsd)
            mk_get "MK_${1}_DISTRO_VERSION"
            __os="freebsd${result%.0}.0"
            ;;
        solaris)
            mk_get "MK_${1}_DISTRO_VERSION"
            __os="solaris2.${result}"
            ;;
        darwin)
            __os="darwin`uname -r`"
            ;;
        aix)
            mk_get "MK_${1}_DISTRO_VERSION"
            __os="aix${result}.0.0"
            ;;
        hpux)
            mk_get "MK_${1}_DISTRO_VERSION"
            __os="hpux${result}"
            ;;
        *)
            __os="unknown"
            ;;
    esac

    case "${2}" in
        x86_32)
            case "$__os" in
                darwin*)
                    __arch="i386-apple"
                    ;;
                *)
                    __arch="i686-pc"
                    ;;
            esac
            ;;
        x86_64)
            case "$__os" in
                darwin*)
                    __arch="x86_64-apple"
                    ;;
                *)
                    __arch="x86_64-unknown"
                    ;;
            esac
            ;;
        ppc32)
            case "$__os" in
                darwin*)
                    __arch="ppc-apple"
                    ;;
                aix*)
                    __arch="powerpc-ibm"
                    ;;
                *)
                    __arch="powerpc-unknown"
                    ;;
            esac
            ;;
        ppc64)
            case "$__os" in
                darwin*)
                    __arch="ppc64-apple"
                    ;;
                aix*)
                    __arch="powerpc-ibm"
                    ;;
                *)
                    __arch="powerpc-unknown"
                    ;;
            esac
            ;;
        sparc*)
            __arch="sparc-sun"
            ;;
        hppa32)
            __arch="hppa2.0-hp"
            ;;
        hppa64)
            __arch="hppa64-hp"
            ;;
        ia64*)
            case "$__os" in
                hpux*)
                    __arch="ia64-hp"
                    ;;
                *)
                    __arch="ia64-unknown"
                    ;;
            esac
            ;;
        *)
            __arch="unknown-unknown"
            ;;
    esac

    result="${__arch}-${__os}"
}

_mk_autotools()
{
    unset _stage_deps

    if [ -n "$SOURCEDIR" ]
    then
        dirname="${MK_SUBDIR:+${MK_SUBDIR#/}/}$SOURCEDIR"
    elif [ -n "$MK_SUBDIR" ]
    then
        dirname="${MK_SUBDIR#/}"
    else
        dirname="$PROJECT_NAME"
    fi
    
    mk_comment "autotools source component $dirname ($SYSTEM)"

    for _lib in ${LIBDEPS}
    do
        if _mk_contains "$_lib" ${MK_INTERNAL_LIBS}
        then
            _stage_deps="$_stage_deps '${MK_LIBDIR}/lib${_lib}.la'"
        fi
    done
    
    for _header in ${HEADERDEPS}
    do
        if _mk_contains "$_header" ${MK_INTERNAL_HEADERS}
        then
            mk_resolve_header "$_header"
            mk_quote "$result"
            _stage_deps="$_stage_deps $result"
        fi
    done

    _mk_slashless_name "${SOURCEDIR:-build}/${CANONICAL_SYSTEM}"
    BUILDDIR="$result"

    mk_resolve_target "$BUILDDIR"
    mk_add_clean_target "$result"

    mk_target \
        SYSTEM="$SYSTEM" \
        TARGET=".${BUILDDIR}_configure" \
        DEPS="$DEPS ${_stage_deps}" \
        mk_run_script \
        at-configure \
        %SOURCEDIR %BUILDDIR %CPPFLAGS %CFLAGS %CXXFLAGS %LDFLAGS \
	%SET_LIBRARY_PATH \
        DIR="$dir" '$@' "*$PARAMS" "*$_MK_AT_PASS_VARS"

    __configure_stamp="$result"
    mk_quote "$result"

    mk_target \
        SYSTEM="$SYSTEM" \
        TARGET=".${BUILDDIR}_build" \
        DEPS="$BUILDDEPS $result" \
        mk_run_script \
        at-build \
        %SOURCEDIR %BUILDDIR %INSTALL %MAKE_BUILD_TARGET \
        MAKE='$(MAKE)' MFLAGS='$(MFLAGS)' '$@'
}

#<
# @brief Build autotools source component
# @usage options... -- configure_params...
# @option SOURCEDIR=dir Specifies where the autotools source
# component is.  Defaults to the directory containing
# MakeKitBuild.
# @option HEADERS=headers Specifies system headers installed
# by the component
# @option LIBS=libs Specifies libraries installed by the component.
# Each library should be specified as its base name with no
# file extension or <lit>lib</lit> prefix.  Each name should be
# followed by a colon (<lit>:</lit>) and a version
# number of the form
# <param>major</param><lit>:</lit><param>minor</param><lit>:</lit><param>micro</param>
# which is the libtool version triple that the library will be built with.
# This allows all of the version links associated
# with the library to be found and properly installed to the staging area.
# If a version number is omitted, only the <lit>.la</lit> file
# (e.g. <lit>libfoo.la</lit>) and the version-less library file
# name (e.g. <lit>libfoo.so</lit>) will be installed.
# @option TARGETS=targets Specifies additional targets which
# are installed by the component
# @option LIBDEPS=libdeps Specifies libraries the component
# depends on
# @option HEADERDEPS=headerdeps Specifies headers the component
# depends on
# @option DEPS=deps Specifies additional dependencies of the
# component
# @option MAKE_BUILD_TARGET=name The target name to pass to make
# when building the component.  Defaults to nothing (e.g. the
# default target in the Makefile, usually "all").
# @option MAKE_INSTALL_TARGET=name The target name to pass to make
# when installing the component.  Defaults to "install".
# @option CPPFLAGS=flags Additional C preprocessor flags
# @option CFLAGS=flags Additional C compiler flags
# @option CXXFLAGS=flags Additional C++ compiler flags
# @option LDFLAGS=flags Additional linker flags
# @option INSTALL_PRE=func Specifies a custom function to run
# before installing the component into a temporary directory.
# The function is passed the path to the temporary install
# directory as the first argument.
# @option INSTALL_POST=func Specifies a custom function to
# run after installing the component into a temporary directory.
# The function is passed the path to the temporary install
# directory as the first arguments
# @option BUILDDEP_PATTERNS=inc_patterns An optional list of patterns
# that will be passed to <lit>find</lit> to identify files within
# the source tree that should trigger a rebuild when changed.
# A reasonable default will be used if not specified that works
# for most C/C++ autotools projects.
# @option BUILDDEP_EXCLUDE_PATTERNS=exc_patterns An optional list
# of patterns that will be passed to <lit>find</lit> to prune when
# looking for files in the source tree.  A reasonable default that
# skips all hidden files and directories will be used if not
# specified.
# @option configure_params Additional parameters to pass to
# the component's configure script.
#
# Builds and installs an autotools (autoconf, automake, libtool) source
# component as part of the MakeKit project.  The component is taken through
# the usual <lit>configure</lit>, <lit>make</lit>, <lit>make install</lit>
# procedure to install it into a temporary location.  All files indicated
# by the <param>libs</param>, <param>headers</param>, and
# <param>targets</param> parameters are then moved into the staging area.
#
# For each library specified in <param>libs</param>, a <lit>.la</lit> file
# will be synthesized if the component did not create one itself.
#
# The remaining positional arguments to this function are passed verbatim
# to the configure script of the component.  In addition, flags such as
# <lit>--prefix</lit> are passed automatically according to how the
# MakeKit project was configured.
#
# @example
# make()
# {
#     # Build popt in the popt-1.15 directory
#     mk_autotools \
#         SOURCEDIR="popt-1.15" HEADERS="popt.h" LIBS="popt" -- \
#         --disable-nls
# }
# @endexample
#>
mk_autotools()
{
    mk_push_vars \
        SOURCEDIR HEADERS LIBS PROGRAMS LIBDEPS HEADERDEPS \
        CPPFLAGS CFLAGS CXXFLAGS LDFLAGS INSTALL TARGETS \
        BUILDDIR DEPS BUILDDEPS SYSTEM="$MK_SYSTEM" CANONICAL_SYSTEM \
        INSTALL_PRE INSTALL_POST SET_LIBRARY_PATH=yes \
	MAKE_BUILD_TARGET="" MAKE_INSTALL_TARGET="install" \
        VERSION MAJOR MINOR MICRO LINKS LIB SONAME EXT="$MK_LIB_EXT" \
        PARAMS EXTRA_TARGETS BUILDDEP_PATTERNS="$_MK_AT_BUILDDEP_PATTERNS" \
        BUILDDEP_EXCLUDE_PATTERNS="$_MK_AT_BUILDDEP_EXCLUDE_PATTERNS" \
        prefix dirname
    mk_parse_params

    mk_quote_list "$@"
    PARAMS="$result"

    _mk_at_expand_srcdir_patterns "$BUILDDEP_PATTERNS" "$BUILDDEP_EXCLUDE_PATTERNS"
    BUILDDEPS="$BUILDDEPS $result"

    # Process and merge targets
    for _header in ${HEADERS}
    do
        MK_INTERNAL_HEADERS="$MK_INTERNAL_HEADERS $_header"

        mk_resolve_header "$_header"
        mk_quote "$result"

        TARGETS="$TARGETS $result"
    done

    for LIB in ${LIBS}
    do
        MK_INTERNAL_LIBS="$MK_INTERNAL_LIBS ${LIB%%:*}"

        mk_resolve_target "${MK_LIBDIR}/lib${LIB%%:*}.la"
        EXTRA_TARGETS="$EXTRA_TARGETS $result"

        case "$LIB" in
            *:*)
                VERSION="${LIB#*:}"
                LIB="${LIB%%:*}"
                _mk_library_process_version
                ;;
            *)
                mk_quote "lib${LIB%%:*}${MK_LIB_EXT}"
                LINKS="$result"
                ;;
        esac
        
        mk_unquote_list "$LINKS"

        for link
        do
            mk_quote "${MK_LIBDIR}/$link"
            TARGETS="$TARGETS $result"
        done
    done

    for _program in ${PROGRAMS}
    do
        MK_INTERNAL_PROGRAMS="$MK_INTERNAL_PROGRAMS $_program"
       
        mk_quote "@${MK_OBJECT_DIR}/build-run/bin/${_program}"
        
        TARGETS="$TARGETS $result"
    done

    mk_resolve_targets "$TARGETS"
    TARGETS="$result"

    if ! [ -d "${MK_SOURCE_DIR}${MK_SUBDIR}/$SOURCEDIR" ]
    then
        mk_quote "$SOURCEDIR"
        DEPS="$DEPS $result"
    elif ! [ -f "${MK_SOURCE_DIR}${MK_SUBDIR}/${SOURCEDIR}/configure" ]
    then
        if [ -f "${MK_SOURCE_DIR}${MK_SUBDIR}/${SOURCEDIR}/autogen.sh" ]
        then
            _command="./autogen.sh"
            _msg="running autogen.sh"
        else
            _command="autoreconf -fi"
            _msg="running autoreconf"
        fi

        if [ -n "$SOURCEDIR" ]
        then
            _msg="$_msg for $SOURCEDIR"
        fi

        mk_msg "$_msg"
        mk_cd_or_fail "${MK_SOURCE_DIR}${MK_SUBDIR}/${SOURCEDIR}"
        mk_run_or_fail ${_command}
        mk_cd_or_fail "${MK_ROOT_DIR}"
    fi
    
    if [ "$MK_SYSTEM" = "host" -a "$MK_HOST_MULTIARCH" = "combine" ]
    then
        parts=""

        for _isa in ${MK_HOST_ISAS}
        do
            SYSTEM="host/$_isa"
            CANONICAL_SYSTEM="$SYSTEM"

            _mk_autotools "$@"
            mk_quote "$result"
            stamp="$result"

            mk_resolve_file ".${BUILDDIR}_install"
            DESTDIR="$result"

            mk_target \
                SYSTEM="$SYSTEM" \
                TARGET="@$DESTDIR" \
                DEPS="$stamp" \
                mk_run_script \
                at-install \
                DESTDIR="$DESTDIR" \
                %SOURCEDIR %BUILDDIR %INSTALL %INSTALL_PRE %INSTALL_POST %MAKE_INSTALL_TARGET \
                MAKE='$(MAKE)' MFLAGS='$(MFLAGS)' '$@' "*$TARGETS $EXTRA_TARGETS"
            mk_quote "$result"
            parts="$parts $result"
        done

        _mk_slashless_name "${SOURCEDIR:-build}_host"

        mk_target \
            TARGET=".${result}_stage" \
            DEPS="$parts" \
            mk_run_script at-combine '$@' "*$parts"
        stamp="$result"
    else
        mk_canonical_system "$SYSTEM"
        CANONICAL_SYSTEM="$result"

        _mk_autotools "$@"
        if [ "$INSTALL" != "no" ]
        then
            mk_quote "$result"
            
            mk_target \
                SYSTEM="$SYSTEM" \
                TARGET=".${BUILDDIR}_stage" \
                DEPS="$result" \
                mk_run_script \
                at-install \
                DESTDIR="${MK_STAGE_DIR}" \
                %SOURCEDIR %BUILDDIR %INSTALL %INSTALL_PRE %INSTALL_POST %MAKE_INSTALL_TARGET \
                MAKE='$(MAKE)' MFLAGS='$(MFLAGS)' '$@' "*$TARGETS $EXTRA_TARGETS"
        fi
        stamp="$result"
    fi

    mk_quote "$stamp"
    quote_stamp="$result"

    mk_unquote_list "$TARGETS"

    for _target
    do
        mk_target \
            TARGET="$_target" \
            DEPS="$quote_stamp"
    done

    # Ensure we get .la files for all libraries
    for _lib in ${LIBS}
    do
        mk_target \
            TARGET="${MK_LIBDIR}/lib${_lib%%:*}.la" \
            DEPS="$quote_stamp" \
            mk_at_la '$@'
    done

    if [ -n "$SOURCEDIR" ]
    then
        # Add convenience rule for building just this component
        mk_target \
            TARGET="@${MK_SUBDIR:+${MK_SUBDIR#/}/}$SOURCEDIR" \
            DEPS="$stamp"
        mk_add_phony_target "$result"
    fi

    result="$stamp"
    mk_pop_vars
}

_mk_at_expand_srcdir_patterns()
{
    _include="$1"
    _exclude="$2"
    _args=""

    set -f
    mk_unquote_list "$_exclude"
    set +f
    for _pattern
    do
        mk_quote_list -o -name "$_pattern"
        _args="$_args $result"
    done
    
    _args="$_args -prune"

    set -f
    mk_unquote_list "$_include"
    set +f
    for _pattern
    do
        mk_quote_list -o -name "$_pattern"
        _args="$_args $result"
    done

    _args="$_args -print"

    mk_unquote_list "$_args"
    shift

    _IFS="$IFS"
    IFS='
'
    set -- `find "$MK_SOURCE_DIR$MK_SUBDIR${SOURCEDIR:+/$SOURCEDIR}" "$@" | sed 's/^/@/g'`
    IFS="$_IFS"

    mk_quote_list "$@"
}

option()
{
    _mk_at_system_string BUILD "${MK_BUILD_PRIMARY_ISA}"

    mk_option \
        OPTION="at-build-string" \
        VAR=MK_AT_BUILD_STRING \
        DEFAULT="$result" \
        HELP="Build system string"

    for _isa in ${MK_HOST_ISAS}
    do   
        _mk_define_name "$_isa"
        _var="MK_AT_HOST_STRING_$result"
        _option="at-host-string-$(echo $_isa | tr '_' '-')"

        _mk_at_system_string HOST "$_isa"

        mk_option \
            OPTION="$_option" \
            VAR="$_var" \
            DEFAULT="$result" \
            HELP="Host system string ($_isa)"
    done

    mk_option \
        OPTION="at-pass-vars" \
        VAR=MK_AT_PASS_VARS \
        DEFAULT="" \
        HELP="List of additional variables to pass when configuring"
}

configure()
{
    mk_msg "build system string: $MK_AT_BUILD_STRING"

    mk_declare -e MK_AT_BUILD_STRING
    mk_declare -s -e MK_AT_HOST_STRING
    
    for _isa in ${MK_HOST_ISAS}
    do
        _mk_define_name "$_isa"
        mk_get "MK_AT_HOST_STRING_$result"
        mk_msg "host system string ($_isa): $result"
        mk_set_system_var SYSTEM="host/$_isa" MK_AT_HOST_STRING "$result"
    done

    mk_msg "pass-through variables: $MK_AT_PASS_VARS"

    _MK_AT_PASS_VARS=""

    for _var in ${MK_AT_PASS_VARS}
    do
        _MK_AT_PASS_VARS="${_MK_AT_PASS_VARS} %$_var"
    done

    _MK_AT_BUILDDEP_PATTERNS="Makefile.am configure.in configure.ac *.c *.h *.cpp *.C *.cp *.s"
    _MK_AT_BUILDDEP_EXCLUDE_PATTERNS=".*"
}

### section build

mk_at_la()
{
    if ! [ -f "$1" ]
    then
        mk_run_script link \
            MODE=la EXT="${MK_LIB_EXT}" "$1"
    else
        mk_run_or_fail touch "$1"
    fi
}

mk_at_log_command()
{
    # $1 = source directory
    # $2 = step

    _mk_slashless_name "$1_$2_$MK_CANONICAL_SYSTEM"
    _log="${MK_ROOT_DIR}/${MK_LOG_DIR}/${result}.log"

    shift 2

    mk_mkdir "${MK_ROOT_DIR}/${MK_LOG_DIR}"

    if [ -n "$MK_VERBOSE" ]
    then
        mk_quote_list "$@"
        mk_msg_verbose "+ $result"
    fi

    if ! "$@" >"$_log" 2>&1
    then
        mk_quote_list "$@"
        mk_msg "FAILED: $result"
        echo ""
        echo "Last 100 lines of ${_log#$MK_ROOT_DIR/}:"
        echo ""
        tail -100 "$_log"
        exit 1
    fi
}
