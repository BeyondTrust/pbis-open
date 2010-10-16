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
	*)
	    __os="unknown"
	    ;;
    esac

    mk_get "MK_${1}_ARCH"

    case "${result}" in
	x86)
	    __arch="i686-pc"
	    ;;
	x86_64)
	    __arch="x86_64-unknown"
	    ;;
	*)
	    __arch="unknown-unknown"
	    ;;
    esac

    result="${__arch}-${__os}"
}

mk_autotools()
{
    mk_push_vars \
	SOURCEDIR HEADERS LIBS PROGRAMS LIBDEPS HEADERDEPS \
	CPPFLAGS CFLAGS LDFLAGS INSTALL TARGETS SELECT \
	BUILDDIR prefix
    mk_parse_params
    
    unset _stage_deps
    
    mk_comment "autotools source component $SOURCEDIR ($MK_SYSTEM)"

    for _lib in ${LIBDEPS}
    do
        if _mk_contains "$_lib" ${MK_INTERNAL_LIBS}
        then
	    _stage_deps="$_stage_deps '${MK_LIBDIR}/lib${_lib}${MK_LIB_EXT}'"
        fi
    done
    
    for _header in ${HEADERDEPS}
    do
        if _mk_contains "$_header" ${MK_INTERNAL_HEADERS}
        then
	    _stage_deps="$_stage_deps '${MK_INCLUDEDIR}/${_header}'"
        fi
    done

    _mk_slashless_name "${SOURCEDIR}/${MK_CANONICAL_SYSTEM}"
    BUILDDIR="$result"

    mk_resolve_target "$BUILDDIR"
    mk_add_clean_target "$result"

    mk_target \
	TARGET=".${BUILDDIR}_configure" \
	DEPS="${_stage_deps}" \
	mk_run_script \
	at-configure \
	%SOURCEDIR %BUILDDIR %CPPFLAGS %CFLAGS %LDFLAGS \
	DIR="$dir" '$@' "$@"

    __configure_stamp="$result"

    mk_target \
	TARGET=".${BUILDDIR}_build" \
	DEPS="'$__configure_stamp'" \
	mk_run_script \
	at-build \
	%SYSTEM %SOURCEDIR %BUILDDIR %INSTALL %SELECT \
	MAKE='$(MAKE)' MFLAGS='$(MFLAGS)' '$@'

    __build_stamp="$result"

    # Add dummy rules for target built by this component
    for _header in ${HEADERS}
    do
	mk_target \
	    TARGET="${MK_INCLUDEDIR}/${_header}" \
	    DEPS="'$__build_stamp'"

	mk_add_all_target "$result"

	MK_INTERNAL_HEADERS="$MK_INTERNAL_HEADERS $_header"
    done

    for _lib in ${LIBS}
    do
	mk_target \
	    TARGET="${MK_LIBDIR}/lib${_lib}${MK_LIB_EXT}" \
	    DEPS="'$__build_stamp'"

	mk_add_all_target "$result"

	MK_INTERNAL_LIBS="$MK_INTERNAL_LIBS $_lib"
    done

    for _program in ${PROGRAMS}
    do
	mk_target \
	    TARGET="@${MK_OBJECT_DIR}/build-run/bin/${_program}" \
	    DEPS="'$__build_stamp'"

	MK_INTERNAL_PROGRAMS="$MK_INTERNAL_PROGRAMS $_program"
    done

    for _target in ${TARGETS}
    do
	mk_target \
	    TARGET="$_target" \
	    DEPS="'$__build_stamp'"
    done

    # Add convenience rule for building just this component
    mk_target \
	TARGET="@${MK_SUBDIR:+${MK_SUBDIR#/}/}$SOURCEDIR" \
	DEPS="$__build_stamp"

    mk_add_phony_target "$result"

    if ! [ -f "${MK_SOURCE_DIR}${MK_SUBDIR}/${SOURCEDIR}/configure" ]
    then
	if [ -f "${MK_SOURCE_DIR}${MK_SUBDIR}/${SOURCEDIR}/autogen.sh" ]
	then
	    mk_msg "running autogen.sh for ${SOURCEDIR}"
	    cd "${MK_SOURCE_DIR}${MK_SUBDIR}/${SOURCEDIR}" && mk_run_or_fail "./autogen.sh"
	    cd "${MK_ROOT_DIR}"
	else
	    mk_msg "running autoreconf for ${SOURCEDIR}"
	    cd "${MK_SOURCE_DIR}${MK_SUBDIR}/${SOURCEDIR}" && mk_run_or_fail autoreconf -fi
	    cd "${MK_ROOT_DIR}"
	fi
    fi

    mk_pop_vars
}

option()
{
    _mk_at_system_string BUILD

    mk_option \
	OPTION="at-build-string" \
	VAR=MK_AT_BUILD_STRING \
	DEFAULT="$result" \
	HELP="Build system string"

    _mk_at_system_string HOST

    mk_option \
	OPTION="at-host-string" \
	VAR=MK_AT_HOST_STRING \
	DEFAULT="$result" \
	HELP="Host system string"
}

configure()
{
    mk_msg "build system string: $MK_AT_BUILD_STRING"
    mk_msg "host system string: $MK_AT_HOST_STRING"

    mk_export MK_AT_BUILD_STRING MK_AT_HOST_STRING
}
