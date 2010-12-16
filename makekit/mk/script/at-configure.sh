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

_stamp="$1"
shift

MK_MSG_DOMAIN="configure"

if [ -n "$SOURCEDIR" ]
then
    dirname="${MK_SUBDIR#/}/$SOURCEDIR"
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
else
    _prefix="$MK_PREFIX"
    _includedir="$MK_INCLUDEDIR"
    _libdir="$MK_LIBDIR"
    _bindir="$MK_BINDIR"
    _sbindir="$MK_SBINDIR"
    _sysconfdir="$MK_SYSCONFDIR"
    _localstatedir="$MK_LOCALSTATEDIR"
fi

_src_dir="`cd ${MK_SOURCE_DIR}${MK_SUBDIR}/${SOURCEDIR} && pwd`"
_stage_dir="`cd ${MK_STAGE_DIR} && pwd`"
_include_dir="${_stage_dir}${_includedir}"
_lib_dir="${_stage_dir}${_libdir}"
_libpath=""

# Make the linker happy, etc.
case "$MK_OS" in
    linux|freebsd)
        _ldflags="-L${_lib_dir} -Wl,-rpath-link -Wl,${_lib_dir}"
        if [ "$MK_CROSS_COMPILING" = "no" ]
        then
            LD_LIBRARY_PATH="$_lib_dir:$LD_LIBRARY_PATH"
            export LD_LIBRARY_PATH
        fi
        ;;
    solaris)
        _ldflags="-L${_lib_dir}"
        if [ "$MK_CROSS_COMPILING" = "no" ]
        then
            LD_LIBRARY_PATH="$_lib_dir:$LD_LIBRARY_PATH"
            export LD_LIBRARY_PATH
        fi
        ;;
    *)
        _ldflags="-L${_lib_dir}"
        ;;
esac

cd "${MK_OBJECT_DIR}${MK_SUBDIR}/$BUILDDIR" && \
mk_run_quiet_or_fail "${_src_dir}/configure" \
    CC="$MK_CC" \
    CXX="$MK_CXX" \
    CPPFLAGS="-I${_include_dir} $_cppflags $CPPFLAGS" \
    CFLAGS="$MK_CFLAGS $CFLAGS" \
    CXXFLAGS="$MK_CXXFLAGS $CXXFLAGS" \
    LDFLAGS="${_ldflags} $MK_LDFLAGS $LDFLAGS" \
    --build="${MK_AT_BUILD_STRING}" \
    --host="${MK_AT_HOST_STRING}" \
    --prefix="${_prefix}" \
    --libdir="${_libdir}" \
    --bindir="${_bindir}" \
    --sbindir="${_sbindir}" \
    --sysconfdir="${_sysconfdir}" \
    --localstatedir="${_localstatedir}" \
    "$@"
cd "${MK_ROOT_DIR}" && mk_run_quiet_or_fail touch "$_stamp"
mk_msg "end ${__msg}"