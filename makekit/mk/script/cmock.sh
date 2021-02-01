#
# Copyright (c) BeyondTrust
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

MK_MSG_DOMAIN="test"

STAGE_LIB_DIR="$1"
shift
SUBDIR="$1"
shift

FAILURES=0
TESTLIST=`find -L ${SUBDIR} -type f -exec test -x {} \; -print `
for t in ${TESTLIST}
do
    mk_msg "${t}"

    TESTDIR=`dirname ${t}`
    TESTNAME=`basename ${t}`

    CWD=${PWD}
    cd ${TESTDIR}
    (export LD_LIBRARY_PATH=${STAGE_LIB_DIR}${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}; export DYLD_LIBRARY_PATH=${STAGE_LIB_DIR}${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}; ./${TESTNAME})
    FAILURES=`expr ${FAILURES} + $?`
    cd ${CWD}
done

mk_msg "CMock/Unity test failures: ${FAILURES}"
exit ${FAILURES}
