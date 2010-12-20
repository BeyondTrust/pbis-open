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

option()
{
    mk_option \
        OPTION=prefix \
        VAR=MK_PREFIX \
        PARAM="path" \
        DEFAULT='/usr/local' \
        HELP="Architecture-independent installation prefix"

    mk_option \
        OPTION=exec-prefix \
        VAR=MK_EPREFIX \
        PARAM="path" \
        DEFAULT="$MK_PREFIX" \
        HELP="Architecture-dependent installation prefix"
    
    if [ "${MK_PREFIX}" = "/usr" ]
    then
        _default_sysconfdir="/etc"
        _default_localstatedir="/var"
    else
        _default_sysconfdir="$MK_PREFIX/etc"
        _default_localstatedir="$MK_PREFIX/var"
    fi

    for _isa in ${MK_HOST_ISAS}
    do
        _mk_define_name "host/$_isa"
        _var="MK_LIBDIR_$result"
        
        case "${MK_HOST_OS}-${MK_HOST_DISTRO_ARCHETYPE}-${MK_HOST_ARCH}-${_isa}" in
            linux-redhat-x86_64-x86_64|linux-suse-x86_64-x86_64)
                _default_libdir="${MK_EPREFIX}/lib64"
                ;;
            linux-debian-x86_64-x86_32)
                _default_libdir="${MK_EPREFIX}/lib32"
                ;;
            solaris-*-sparc*-sparc_64)
                _default_libdir="${MK_EPREFIX}/lib/sparcv9"
                ;;
            solaris-*-x86_64-x86_64)
                _default_libdir="${MK_EPREFIX}/lib/64"
                ;;
            *)
                _default_libdir="${MK_EPREFIX}/lib"
                ;;
        esac

        if [ "$_isa" = "${MK_HOST_ISAS% *}" ]
        then
            _option="libdir"
        else
            _option="libdir-$(echo $_isa | tr '_' '-')"
        fi

        mk_option \
            OPTION="$_option" \
            VAR="$_var"  \
            PARAM="path" \
            DEFAULT="$_default_libdir" \
            HELP="Library directory ($_isa)"
    done

    mk_option \
        VAR=MK_INCLUDEDIR \
        OPTION=includedir \
        PARAM="path" \
        DEFAULT="${MK_PREFIX}/include" \
        HELP="Header file directory"

    mk_option \
        VAR=MK_BINDIR \
        OPTION=bindir \
        PARAM="path" \
        DEFAULT="${MK_EPREFIX}/bin" \
        HELP="User executable directory"

    mk_option \
        VAR=MK_SBINDIR \
        OPTION=sbindir \
        PARAM="path" \
        DEFAULT="${MK_EPREFIX}/sbin" \
        HELP="System executable directory"

    mk_option \
        VAR=MK_LIBEXECDIR \
        OPTION=libexecdir \
        PARAM="path" \
        DEFAULT="${MK_EPREFIX}/libexec" \
        HELP="Program executable directory"

    mk_option \
        VAR=MK_SYSCONFDIR \
        OPTION=sysconfdir \
        PARAM="path" \
        DEFAULT="${_default_sysconfdir}" \
        HELP="System configuration directory"
    
    mk_option \
        VAR=MK_LOCALSTATEDIR \
        OPTION=localstatedir \
        PARAM="path" \
        DEFAULT="${_default_localstatedir}" \
        HELP="Local state directory"

    mk_option \
        VAR=MK_DATAROOTDIR \
        OPTION=datarootdir \
        PARAM="path" \
        DEFAULT="${MK_PREFIX}/share" \
        HELP="Root data directory"
    
    mk_option \
        VAR=MK_DATADIR \
        OPTION=datadir \
        PARAM="path" \
        DEFAULT="${MK_DATAROOTDIR}/${PROJECT_NAME}" \
        HELP="Data directory"

    mk_option \
        VAR=MK_DOCDIR \
        OPTION=docdir \
        PARAM="path" \
        DEFAULT="${MK_DATAROOTDIR}/doc/${PROJECT_NAME}" \
        HELP="Documentation directory"

    mk_option \
        VAR=MK_HTMLDIR \
        OPTION=htmldir \
        PARAM="path" \
        DEFAULT="${MK_DOCDIR}/html" \
        HELP="HTML documentation directory"

    mk_option \
        VAR=MK_MANDIR \
        OPTION=mandir \
        PARAM="path" \
        DEFAULT="${MK_DATAROOTDIR}/man" \
        HELP="UNIX man page directory"
}

configure()
{
    mk_declare_system_var MK_LIBDIR

    mk_msg "prefix: $MK_PREFIX"
    mk_msg "exec prefix: $MK_EPREFIX"

    for _isa in ${MK_HOST_ISAS}
    do
        _mk_define_name "host/$_isa"
        _var="MK_LIBDIR_$result"
        _vars="$_vars $_var"
        mk_get "$_var"
        mk_msg "library dir ($_isa): $result"
        mk_set_system_var SYSTEM="host/$_isa" MK_LIBDIR "$result"
    done
    
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

    mk_export \
        MK_PREFIX MK_EPREFIX MK_LIBDIR MK_INCLUDEDIR MK_BINDIR \
        MK_SBINDIR MK_LIBEXECDIR MK_SYSCONFDIR MK_LOCALSTATEDIR \
        MK_DATAROOTDIR MK_DATADIR MK_DOCDIR MK_HTMLDIR \
        MK_MANDIR

    # Set up paths where programs compiled for the build system should go
    mk_export MK_RUN_PREFIX="${MK_RUN_DIR}"
    mk_export MK_RUN_EPREFIX="${MK_RUN_DIR}"
    mk_export MK_RUN_LIBDIR="${MK_RUN_DIR}/lib"
    mk_export MK_RUN_BINDIR="${MK_RUN_DIR}/bin"
    mk_export MK_RUN_SBINDIR="${MK_RUN_DIR}/sbin"
    mk_export MK_RUN_SYSCONFDIR="${MK_RUN_DIR}/etc"
    mk_export MK_RUN_LOCALSTATEDIR="${MK_RUN_DIR}/var"
    mk_export MK_RUN_DATAROOTDIR="${MK_RUN_DIR}/share"
    mk_export MK_RUN_DATADIR="${MK_RUN_DIR}/share"
}
