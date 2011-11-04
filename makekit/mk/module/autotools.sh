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
                    __arch="ppc-unknown"
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
                    __arch="ppc64-unknown"
                    ;;
            esac
            ;;
        sparc*)
            __arch="sparc-sun"
            ;;
        hppa*)
            __arch="hppa2.0-hp"
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
        DIR="$dir" '$@' "$@" "*$_MK_AT_PASS_VARS"

    __configure_stamp="$result"

    mk_target \
        SYSTEM="$SYSTEM" \
        TARGET=".${BUILDDIR}_build" \
        DEPS="'$__configure_stamp'" \
        mk_run_script \
        at-build \
        %SOURCEDIR %BUILDDIR %INSTALL %MAKE_BUILD_TARGET \
        MAKE='$(MAKE)' MFLAGS='$(MFLAGS)' '$@'
}

mk_autotools()
{
    mk_push_vars \
        SOURCEDIR HEADERS LIBS PROGRAMS LIBDEPS HEADERDEPS \
        CPPFLAGS CFLAGS CXXFLAGS LDFLAGS INSTALL TARGETS SELECT \
        BUILDDIR DEPS SYSTEM="$MK_SYSTEM" CANONICAL_SYSTEM \
        INSTALL_PRE INSTALL_POST SET_LIBRARY_PATH=yes \
	MAKE_BUILD_TARGET="" MAKE_INSTALL_TARGET="install" \
        prefix dirname
    mk_parse_params
    
    if ! [ -d "${MK_SOURCE_DIR}${MK_SUBDIR}/$SOURCEDIR" ]
    then
        mk_quote "$SOURCEDIR"
        DEPS="$DEPS $result"
    elif ! [ -f "${MK_SOURCE_DIR}${MK_SUBDIR}/${SOURCEDIR}/configure" ]
    then
        if [ -f "${MK_SOURCE_DIR}${MK_SUBDIR}/${SOURCEDIR}/autogen.sh" ]
        then
            mk_msg "running autogen.sh for ${dirname}"
            cd "${MK_SOURCE_DIR}${MK_SUBDIR}/${SOURCEDIR}" && mk_run_or_fail "./autogen.sh"
            cd "${MK_ROOT_DIR}"
        else
            mk_msg "running autoreconf for ${dirname}"
            cd "${MK_SOURCE_DIR}${MK_SUBDIR}/${SOURCEDIR}" && mk_run_or_fail autoreconf -fi
            cd "${MK_ROOT_DIR}"
        fi
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
                %SOURCEDIR %BUILDDIR %INSTALL %SELECT %INSTALL_PRE %INSTALL_POST %MAKE_INSTALL_TARGET \
                MAKE='$(MAKE)' MFLAGS='$(MFLAGS)' '$@'
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
                %SOURCEDIR %BUILDDIR %INSTALL %SELECT %INSTALL_PRE %INSTALL_POST %MAKE_INSTALL_TARGET \
                MAKE='$(MAKE)' MFLAGS='$(MFLAGS)' '$@'
        fi
        stamp="$result"
    fi

    mk_add_all_target "$stamp"

    # Add dummy rules for target built by this component
    for _header in ${HEADERS}
    do
        mk_resolve_header "$_header"
        
        mk_target \
            TARGET="$result" \
            DEPS="'$stamp'"

        mk_add_all_target "$result"

        MK_INTERNAL_HEADERS="$MK_INTERNAL_HEADERS $_header"
    done

    for _lib in ${LIBS}
    do
        mk_target \
            TARGET="${MK_LIBDIR}/lib${_lib}.la" \
            DEPS="'$stamp'" \
            mk_at_la '$@'

        mk_add_all_target "$result"

        MK_INTERNAL_LIBS="$MK_INTERNAL_LIBS $_lib"
    done

    for _program in ${PROGRAMS}
    do
        mk_target \
            TARGET="@${MK_OBJECT_DIR}/build-run/bin/${_program}" \
            DEPS="'$stamp'"

        MK_INTERNAL_PROGRAMS="$MK_INTERNAL_PROGRAMS $_program"
    done

    for _target in ${TARGETS}
    do
        mk_target \
            TARGET="$_target" \
            DEPS="'$stamp'"
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

        # If the build system supports the ISA,
        # make the host string match the build string.
        # This avoids triggering the 'cross compiling'
        # check in many projects when building x86_32
        # on x86_64, etc.
        if [ "$MK_HOST_OS" = "$MK_BUILD_OS" ] && _mk_contains "$_isa" ${MK_BUILD_ISAS}
        then
            result="$MK_AT_BUILD_STRING"
        else
            _mk_at_system_string HOST "$_isa"
        fi

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

    mk_export MK_AT_BUILD_STRING

    mk_declare_system_var MK_AT_HOST_STRING
    
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
}

### section build

mk_at_la()
{
    if ! [ -f "$1" ]
    then
        mk_run_script link \
            MODE=la EXT="${MK_LIB_EXT}" "$1"
    fi
}