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

mk_msg_domain "stage"

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

_stamp="$1"
mk_mkdir "${DESTDIR}"
_stage_dir="`cd "${DESTDIR}" && pwd`"

if [ -d "$MK_RUN_BINDIR" ]
then
    PATH="`cd $MK_RUN_BINDIR && pwd`:$PATH"
    export PATH
fi

case "$MK_OS:$MK_ISA" in
    aix:ppc32)
        OBJECT_MODE="32"
        export OBJECT_MODE
        ;;
    aix:ppc64)
        OBJECT_MODE="64"
        export OBJECT_MODE
        ;;
esac

cd "${MK_OBJECT_DIR}${MK_SUBDIR}/$BUILDDIR" || mk_fail "could not change directory"
if [ "${MK_SYSTEM%/*}" = "build" ]
then
    if [ -n "$INSTALL_PRE" ]
    then
        ${INSTALL_PRE} "${MK_ROOT_DIR}/${MK_RUN_DIR}"
    fi
    mk_run_quiet_or_fail ${MAKE} ${MFLAGS} ${MAKE_INSTALL_TARGET}
    if [ -n "$INSTALL_POST" ]
    then
        ${INSTALL_POST} "${MK_ROOT_DIR}/${MK_RUN_DIR}"
    fi
elif [ -n "$SELECT" ]
then
    # We have to install to a temporary location, then copy selected files
    rm -rf ".install"
    if [ -n "$INSTALL_PRE" ]
    then
        ${INSTALL_PRE} "${PWD}/.install"
    fi
    mk_run_quiet_or_fail ${MAKE} ${MFLAGS} DESTDIR="${PWD}/.install" ${MAKE_INSTALL_TARGET}
    if [ -n "$INSTALL_POST" ]
    then
        ${INSTALL_POST} "${PWD}/.install"
    fi
    mk_expand_absolute_pathnames "$SELECT" ".install"
    mk_unquote_list "$result"
    for _file in "$@"
    do
        if [ -e ".install${_file}" ]
        then
            _dest="${_stage_dir}${_file}"
            mk_mkdir "${_dest%/*}"
            cp -pr ".install${_file}" "$_dest" || mk_fail "failed to copy file: $_file"
        else
            mk_fail "could not select file: $_file"
        fi
    done
    rm -rf ".install"
else
    if [ -n "$INSTALL_PRE" ]
    then
        ${INSTALL_PRE} "${_stage_dir}"
    fi
    mk_run_quiet_or_fail ${MAKE} ${MFLAGS} DESTDIR="${_stage_dir}" ${MAKE_INSTALL_TARGET}
    if [ -n "$INSTALL_POST" ]
    then
        ${INSTALL_POST} "${_stage_dir}"
    fi
fi

cd "${MK_ROOT_DIR}"

if [ -n "$INSTALL_POST" ]
then
    ${INSTALL_POST}
fi

mk_run_or_fail touch "$_stamp"
mk_msg "end ${__msg}"