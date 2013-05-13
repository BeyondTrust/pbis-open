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

hack_libtool()
{
    case "$MK_OS:$MK_ARCH" in
        hpux:ia64|darwin:*|freebsd:*)
            if [ -x libtool ]
            then
                sed \
                    -e 's/^hardcode_direct=no/hardcode_direct=yes/' \
                    -e 's/^hardcode_direct_absolute=no/hardcode_direct_absolute=yes/' \
                    -e 's/^hardcode_libdir_flag_spec=.*/hardcode_libdir_flag_spec=""/' \
                    < libtool > libtool.new
                mv -f libtool.new libtool
                chmod +x libtool
            fi
            ;;
        *)
            if [ -x libtool ]
            then
                sed \
                    -e 's/^hardcode_libdir_flag_spec=.*/hardcode_libdir_flag_spec=""/' \
                    < libtool > libtool.new
                mv -f libtool.new libtool
                chmod +x libtool
            fi
            ;;
    esac
}

_stamp="$1"
shift

MK_MSG_DOMAIN="configure"

if [ -n "$SOURCEDIR" ]
then
    dirname="${MK_SUBDIR:+${MK_SUBDIR#/}/}$SOURCEDIR"
elif [ -n "$MK_SUBDIR" ]
then
    dirname="${MK_SUBDIR#/}"
else
    dirname="$PROJECT_NAME"
fi

__msg="$dirname ($MK_CANONICAL_SYSTEM)"

mk_msg "begin ${__msg}"

mk_mkdir "${MK_OBJECT_DIR}${MK_SUBDIR}/$BUILDDIR"
mk_mkdir "${MK_STAGE_DIR}"

if [ "${MK_SYSTEM%/*}" = "build" ]
then
    _prefix="$MK_ROOT_DIR/$MK_RUN_PREFIX"
    _includedir="$MK_ROOT_DIR/$MK_RUN_INCLUDEDIR"
    _libdir="$MK_ROOT_DIR/$MK_RUN_LIBDIR"
    _bindir="$MK_ROOT_DIR/$MK_RUN_BINDIR"
    _sbindir="$MK_ROOT_DIR/$MK_RUN_SBINDIR"
    _sysconfdir="$MK_ROOT_DIR/$MK_RUN_SYSCONFDIR"
    _localstatedir="$MK_ROOT_DIR/$MK_RUN_LOCALSTATEDIR"
    _stage_dir="`cd ${MK_STAGE_DIR} && pwd`"
    _include_dir="$_includedir"
    _lib_dir="$_libdir"
else
    _prefix="$MK_PREFIX"
    _includedir="$MK_INCLUDEDIR"
    _libdir="$MK_LIBDIR"
    _bindir="$MK_BINDIR"
    _sbindir="$MK_SBINDIR"
    _sysconfdir="$MK_SYSCONFDIR"
    _localstatedir="$MK_LOCALSTATEDIR"
    _stage_dir="`cd ${MK_STAGE_DIR} && pwd`"
    _include_dir="${_stage_dir}${_includedir}"
    _lib_dir="${_stage_dir}${_libdir}"
fi

mk_resolve_file "$SOURCEDIR"
_src_dir="`cd $result && pwd`"
_libpath=""

# Make the linker happy, etc.
case "$MK_OS" in
    linux|freebsd)
        _ldflags="-L${_lib_dir} -Wl,-rpath-link -Wl,${_lib_dir}"
        if [ "$MK_CROSS_COMPILING" = "no" -a "$SET_LIBRARY_PATH" = "yes" ]
        then
            LD_LIBRARY_PATH="$_lib_dir:$LD_LIBRARY_PATH"
            export LD_LIBRARY_PATH
        fi
        ;;
    solaris)
        _ldflags="-L${_lib_dir}"
        if [ "$MK_CROSS_COMPILING" = "no" -a "$SET_LIBRARY_PATH" = "yes" ]
        then
            LD_LIBRARY_PATH="$_lib_dir:$LD_LIBRARY_PATH"
            export LD_LIBRARY_PATH
        fi
        ;;
    aix)
        _ldflags="-L${_lib_dir} -Wl,-brtl"
        if [ "$MK_CROSS_COMPILING" = "no" -a "$SET_LIBRARY_PATH" = "yes" ]
        then
            LIBPATH="$_lib_dir:$LD_LIBRARY_PATH"
            export LIBPATH
        fi
        ;;
    hpux)
        _ldflags="-L${_lib_dir}"
        if [ "$MK_CROSS_COMPILING" = "no" -a "$SET_LIBRARY_PATH" = "yes" ]
        then
            SHLIB_PATH="$_lib_dir:$SHLIB_PATH"
            export SHLIB_PATH
        fi
        ;;
    darwin)
        DYLD_LIBRARY_PATH="$_lib_dir:$DYLD_LIBRARY_PATH"
        export DYLD_LIBRARY_PATH
        _ldflags="-L${_lib_dir}"
        ;;
    *)
        _ldflags="-L${_lib_dir}"
        ;;
esac

# We disable libtool setting the rpath, so do it ourselves
case "${MK_OS}:${MK_CC_LD_STYLE}" in
    *:gnu)
        _rpath_flags="-Wl,-rpath,${_libdir}"
        ;;
    solaris:native)
        _rpath_flags="-R${_libdir}"
        ;;
    aix:native)
        _rpath_flags="-Wl,-blibpath:${_libdir}:/usr/lib:/lib"
        ;;
    hpux:native)
        _rpath_flags="-Wl,+b,${_libdir}"
        ;;
    *)
        _rpath_flags=""
        ;;
esac

if [ -d "$MK_RUN_BINDIR" ]
then
    PATH="`cd $MK_RUN_BINDIR && pwd`:$PATH"
    export PATH
fi

# If the build system supports the host ISA we will build for,
# pretend that the build system is the same.  This avoids making
# autoconf believe we are cross compiling and failing any run
# tests.
if [ "$MK_HOST_OS" = "$MK_BUILD_OS" ] && _mk_contains "$MK_ISA" ${MK_BUILD_ISAS}
then
    _build_string="$MK_AT_HOST_STRING"
else
    _build_string="$MK_AT_BUILD_STRING"
fi


cd "${MK_OBJECT_DIR}${MK_SUBDIR}/$BUILDDIR" && \
mk_at_log_command "$dirname" "configure" "${_src_dir}/configure" \
    CC="$MK_CC" \
    CXX="$MK_CXX" \
    CPPFLAGS="-I${_include_dir} $_cppflags $CPPFLAGS" \
    CFLAGS="$MK_ISA_CFLAGS $MK_CFLAGS $CFLAGS" \
    CXXFLAGS="$MK_ISA_CXXFLAGS $MK_CXXFLAGS $CXXFLAGS" \
    LDFLAGS="$MK_ISA_LDFLAGS $MK_LDFLAGS $LDFLAGS ${_ldflags} ${_rpath_flags}" \
    --build="${_build_string}" \
    --host="${MK_AT_HOST_STRING}" \
    --prefix="${_prefix}" \
    --libdir="${_libdir}" \
    --bindir="${_bindir}" \
    --sbindir="${_sbindir}" \
    --sysconfdir="${_sysconfdir}" \
    --localstatedir="${_localstatedir}" \
    --enable-fast-install \
    --disable-rpath \
    "$@"

# Does what it says
hack_libtool

cd "${MK_ROOT_DIR}" && mk_run_quiet_or_fail touch "$_stamp"
mk_msg "end ${__msg}"