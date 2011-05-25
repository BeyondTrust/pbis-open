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

MK_MSG_DOMAIN="build"

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
mk_mkdir "${MK_STAGE_DIR}"
_stage_dir="`cd "${MK_STAGE_DIR}" && pwd`"

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
mk_at_log_command "$dirname" "build" ${MAKE} ${MFLAGS} ${MAKE_BUILD_TARGET}
cd "${MK_ROOT_DIR}"
mk_run_or_fail touch "$_stamp"
mk_msg "end ${__msg}"