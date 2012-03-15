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
# path.sh -- UNIX installation path configuration
#
##

### section configure

DEPENDS="platform"

#<
# @var MK_PREFIX
# @brief Install prefix for architecture-independent files
# @export
# 
# The filesystem prefix for architecture-indepenent files.
# Used to construct the default install directories for
# headers, shared data files, configuration files, etc.
#>

#<
# @var MK_EPREFIX
# @brief Install prefix for architecture-dependent files
# @export
# 
# The filesystem prefix for architecture-depenent files.
# Used to construct the default install directories for
# executables, libraries, etc.
#
# Defaults to <lit>$<varref>MK_PREFIX</varref></lit>.
#>

#<
# @var MK_INCLUDEDIR
# @brief Install directory for header files
# @export
# 
# The directory where public header files are installed.
#
# Defaults to <lit>$<varref>MK_PREFIX</varref>/include</lit>.
#>

#<
# @var MK_BINDIR
# @brief Install directory for user executables
# @export
# 
# The directory where programs intended for use
# by users are installed.
#
# Defaults to <lit>$<varref>MK_EPREFIX</varref>/bin</lit>.
#>

#<
# @var MK_SBINDIR
# @brief Install directory for system executables
# @export
# 
# The directory where programs intended for use
# by the operating system or root user are installed.
#
# Defaults to <lit>$<varref>MK_EPREFIX</varref>/sbin</lit>.
#>

#<
# @var MK_LIBEXECDIR
# @brief Install directory for program executables
# @export
# 
# The directory where programs intended for internal
# use by other programs or libraries are installed.
#
# Defaults to <lit>$<varref>MK_EPREFIX</varref>/libexec</lit>.
#>

#<
# @var MK_BASELIBDIR
# @brief Base install directory for libraries
# @export
# 
# This path name is used as a template to construct the actual
# install directory paths for libraries (<varref>MK_LIBDIR</varref>).
# This distinction is important on multiarchitecture systems where
# there may be more than one library directory.
#
# Defaults to <lit>$<varref>MK_EPREFIX</varref>/lib</lit>.
#>

#<
# @var MK_LIBDIR
# @brief Install directory for libraries
# @export
# @system
# 
# The directory where libraries are installed.  On multiarchitecture
# systems that do not use "fat" binaries, this is a system-dependent
# variable that has separate values for each ISA.  For example, on
# a Debian-like x86_64 Linux system, it defaults to <lit>$<varref>MK_BASELIBDIR</varref></lit>
# for the x86_64 ISA and <lit>${<varref>MK_BASELIBDIR</varref>}32</lit> for the
# x86_32 ISA.
#
# On systems that use "fat" binaries, such as Darwin, it is equivalent
# to <varref>MK_BASELIBDIR</varref>.
#>

#<
# @var MK_SYSCONFDIR
# @brief System configuration directory
# @export
#
# The directory where configuration files are installed.
#
# Defaults to <lit>$<varref>MK_PREFIX</varref>/etc</lit>,
# unless <varref>MK_PREFIX</varref> is <lit>/usr</lit>,
# in which case it defaults to <lit>/etc</lit>.
#>

#<
# @var MK_LOCALSTATEDIR
# @brief Local state directory
# @export
#
# The directory where programs should store machine-local
# state information.
#
# Defaults to <lit>$<varref>MK_PREFIX</varref>/var</lit>,
# unless <varref>MK_PREFIX</varref> is <lit>/usr</lit>,
# in which case it defaults to <lit>/var</lit>.
#>

#<
# @var MK_DATAROOTDIR
# @brief Root shared data directory
# @export
#
# The directory where architecture-independent data files
# are installed.
#
# Defaults to <lit>$<varref>MK_PREFIX</varref>/share</lit>.
#>

#<
# @var MK_DATADIR
# @brief Shared data directory
# @export
#
# The directory where architecture-independent data files
# are installed for the current project.
#
# Defaults to <lit>$<varref>MK_DATAROOTDIR</varref>/$PROJECT_NAME</lit>.
#>

#<
# @var MK_DOCDIR
# @brief Documentation directory
# @export
#
# The directory where documentation is installed.
#
# Defaults to <lit>$<varref>MK_DATAROOTDIR</varref>/doc/$PROJECT_NAME</lit>.
#>

#<
# @var MK_HTMLDIR
# @brief HTML documentation directory
# @export
#
# The directory where HTML documentation is installed.
#
# Defaults to <lit>$<varref>MK_DOCDIR</varref>/html</lit>.
#>

#<
# @var MK_MANDIR
# @brief UNIX man page directory
# @export
#
# The directory where UNIX man pages are installed.
#
# Defaults to <lit>$<varref>MK_DATAROOTDIR</varref>/man</lit>.
#>

option()
{
    mk_option \
        OPTION=prefix \
        VAR=PREFIX \
        PARAM="path" \
        DEFAULT='/usr/local' \
        HELP="Architecture-independent installation prefix"

    if [ "${PREFIX}" = "/usr" ]
    then
        _default_sysconfdir="/etc"
        _default_localstatedir="/var"
    else
        _default_sysconfdir="$PREFIX/etc"
        _default_localstatedir="$PREFIX/var"
    fi

    mk_option \
        OPTION=exec-prefix \
        VAR=EPREFIX \
        PARAM="path" \
        DEFAULT="$PREFIX" \
        HELP="Architecture-dependent installation prefix"
    
    mk_option \
        VAR=INCLUDEDIR \
        OPTION=includedir \
        PARAM="path" \
        DEFAULT="${PREFIX}/include" \
        HELP="Header file directory"

    mk_option \
        VAR=BINDIR \
        OPTION=bindir \
        PARAM="path" \
        DEFAULT="${EPREFIX}/bin" \
        HELP="User executable directory"

    mk_option \
        VAR=SBINDIR \
        OPTION=sbindir \
        PARAM="path" \
        DEFAULT="${EPREFIX}/sbin" \
        HELP="System executable directory"

    mk_option \
        VAR=LIBEXECDIR \
        OPTION=libexecdir \
        PARAM="path" \
        DEFAULT="${EPREFIX}/libexec" \
        HELP="Program executable directory"

    mk_option \
        VAR=BASELIBDIR \
        OPTION=libdir \
        PARAM="path" \
        DEFAULT="${EPREFIX}/lib" \
        HELP="Library directory (base)"

    if [ "$MK_HOST_MULTIARCH" = "separate" ]
    then
        for _isa in ${MK_HOST_ISAS}
        do
            _mk_define_name "host/$_isa"
            _var="LIBDIR_$result"
            
            case "${MK_HOST_OS}-${MK_HOST_DISTRO_ARCHETYPE}-${MK_HOST_ARCH}-${_isa}" in
                linux-redhat-x86_64-x86_64|linux-suse-x86_64-x86_64|aix-*-powerpc-ppc64)
                    _default_libdir="${BASELIBDIR}64"
                    ;;
                linux-debian-x86_64-x86_32)
                    _default_libdir="${BASELIBDIR}32"
                    ;;
                solaris-*-sparc*-sparc_64)
                    _default_libdir="${BASELIBDIR}/sparcv9"
                    ;;
                solaris-*-x86_64-x86_64)
                    _default_libdir="${BASELIBDIR}/amd64"
                    ;;
                hpux-*-hppa2.0-hppa64)
                    _default_libdir="${BASELIBDIR}/pa20_64"
                    ;;
                hpux-*-ia64-ia64_32)
                    _default_libdir="${BASELIBDIR}/hpux32"
                    ;;
                hpux-*-ia64-ia64_64)
                    _default_libdir="${BASELIBDIR}/hpux64"
                    ;;
                *)
                    _default_libdir="${BASELIBDIR}"
                    ;;
            esac
            
            _option="libdir-$(echo $_isa | tr '_' '-')"
            
            mk_option \
                OPTION="$_option" \
                VAR="$_var"  \
                PARAM="path" \
                DEFAULT="$_default_libdir" \
                HELP="Library directory ($_isa)"
        done
    fi

    mk_option \
        VAR=SYSCONFDIR \
        OPTION=sysconfdir \
        PARAM="path" \
        DEFAULT="${_default_sysconfdir}" \
        HELP="System configuration directory"
    
    mk_option \
        VAR=LOCALSTATEDIR \
        OPTION=localstatedir \
        PARAM="path" \
        DEFAULT="${_default_localstatedir}" \
        HELP="Local state directory"

    mk_option \
        VAR=DATAROOTDIR \
        OPTION=datarootdir \
        PARAM="path" \
        DEFAULT="${PREFIX}/share" \
        HELP="Root data directory"
    
    mk_option \
        VAR=DATADIR \
        OPTION=datadir \
        PARAM="path" \
        DEFAULT="${DATAROOTDIR}/${PROJECT_NAME}" \
        HELP="Data directory"

    mk_option \
        VAR=DOCDIR \
        OPTION=docdir \
        PARAM="path" \
        DEFAULT="${DATAROOTDIR}/doc/${PROJECT_NAME}" \
        HELP="Documentation directory"

    mk_option \
        VAR=HTMLDIR \
        OPTION=htmldir \
        PARAM="path" \
        DEFAULT="${DOCDIR}/html" \
        HELP="HTML documentation directory"

    mk_option \
        VAR=MANDIR \
        OPTION=mandir \
        PARAM="path" \
        DEFAULT="${DATAROOTDIR}/man" \
        HELP="UNIX man page directory"
}

configure()
{
    # Set up paths where programs compiled for the build system should go
    mk_declare -e \
        MK_RUN_PREFIX="${MK_RUN_DIR}" \
        MK_RUN_EPREFIX="${MK_RUN_DIR}" \
        MK_RUN_INCLUDEDIR="${MK_RUN_DIR}/include" \
        MK_RUN_LIBDIR="${MK_RUN_DIR}/lib" \
        MK_RUN_LIBEXECDIR="${MK_RUN_DIR}/libexec" \
        MK_RUN_BINDIR="${MK_RUN_DIR}/bin" \
        MK_RUN_SBINDIR="${MK_RUN_DIR}/sbin" \
        MK_RUN_SYSCONFDIR="${MK_RUN_DIR}/etc" \
        MK_RUN_LOCALSTATEDIR="${MK_RUN_DIR}/var" \
        MK_RUN_DATAROOTDIR="${MK_RUN_DIR}/share" \
        MK_RUN_DATADIR="${MK_RUN_DIR}/share"

    # Declare basic path variables
    mk_declare -s -e \
        MK_PREFIX MK_EPREFIX MK_INCLUDEDIR MK_BINDIR MK_SBINDIR MK_LIBDIR \
        MK_LIBEXECDIR MK_SYSCONFDIR MK_LOCALSTATEDIR MK_DATAROOTDIR MK_DATADIR

    # Set values of basic path variables
    mk_set_system_var SYSTEM="build" MK_PREFIX "@$MK_RUN_PREFIX"
    mk_set_system_var SYSTEM="build" MK_EPREFIX "@$MK_RUN_EPREFIX"
    mk_set_system_var SYSTEM="build" MK_INCLUDEDIR "@$MK_RUN_INCLUDEDIR"
    mk_set_system_var SYSTEM="build" MK_LIBDIR "@$MK_RUN_LIBDIR"
    mk_set_system_var SYSTEM="build" MK_LIBEXECDIR "@$MK_RUN_LIBEXECDIR"
    mk_set_system_var SYSTEM="build" MK_BINDIR "@$MK_RUN_BINDIR"
    mk_set_system_var SYSTEM="build" MK_SBINDIR "@$MK_RUN_SBINDIR"
    mk_set_system_var SYSTEM="build" MK_SYSCONFDIR "@$MK_RUN_SYSCONFDIR"
    mk_set_system_var SYSTEM="build" MK_LOCALSTATEDIR "@$MK_RUN_LOCALSTATEDIR"
    mk_set_system_var SYSTEM="build" MK_DATAROOTDIR "@$MK_RUN_DATAROOTDIR"
    mk_set_system_var SYSTEM="build" MK_DATADIR "@$MK_RUN_DATADIR"

    mk_set_system_var SYSTEM="host" MK_PREFIX "$PREFIX"
    mk_set_system_var SYSTEM="host" MK_EPREFIX "$EPREFIX"
    mk_set_system_var SYSTEM="host" MK_INCLUDEDIR "$INCLUDEDIR"
    mk_set_system_var SYSTEM="host" MK_BINDIR "$BINDIR"
    mk_set_system_var SYSTEM="host" MK_LIBEXECDIR "$LIBEXECDIR"
    mk_set_system_var SYSTEM="host" MK_SBINDIR "$SBINDIR"
    mk_set_system_var SYSTEM="host" MK_SYSCONFDIR "$SYSCONFDIR"
    mk_set_system_var SYSTEM="host" MK_LOCALSTATEDIR "$LOCALSTATEDIR"
    mk_set_system_var SYSTEM="host" MK_DATAROOTDIR "$DATAROOTDIR"
    mk_set_system_var SYSTEM="host" MK_DATADIR "$DATADIR"

    mk_declare -e \
        MK_DOCDIR="$DOCDIR" MK_MANDIR="$MANDIR" MK_HTMLDIR="$HTMLDIR"

    mk_msg "prefix: $MK_PREFIX"
    mk_msg "exec prefix: $MK_EPREFIX"

    if [ "$MK_HOST_MULTIARCH" = "separate" ]
    then
        for _isa in ${MK_HOST_ISAS}
        do
            _mk_define_name "host/$_isa"
            _var="LIBDIR_$result"
            _vars="$_vars $_var"
            mk_get "$_var"
            mk_msg "library dir ($_isa): $result"
            mk_set_system_var SYSTEM="host/$_isa" MK_LIBDIR "$result"
        done
    else
        mk_msg "library dir: $BASELIBDIR"
        mk_set_system_var SYSTEM="host" MK_LIBDIR "$BASELIBDIR"
    fi

    mk_msg "include dir: $MK_INCLUDEDIR"
    mk_msg "binary dir: $MK_BINDIR"
    mk_msg "system binary dir: $MK_SBINDIR"
    mk_msg "system config dir: $MK_SYSCONFDIR"
    mk_msg "local state dir: $MK_LOCALSTATEDIR"
    mk_msg "data root dir: $MK_DATAROOTDIR"
    mk_msg "data dir: $MK_DATADIR"
    mk_msg "documenation dir: $MK_DOCDIR"
    mk_msg "HTML documentation dir: $MK_HTMLDIR"
    mk_msg "manpage dir: $MK_MANDIR"
}
