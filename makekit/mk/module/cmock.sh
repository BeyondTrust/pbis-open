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

##
#
# cmock.sh -- Support for building CMock/Unity unit tests
#   This includes:
#     - checking for the presence of the AD Bridge packaged cmock/unity library
#     - building a test; basically a glorified mk_program
#     - running the list of tests
#
# Example
#
#    mk_have_cmock && mk_cmock
#       PROGRAM="testparser"  \
#       SOURCES="parser.c mocks/MockParser.c testparser.c" \
#       INCLUDEDIRS=". mocks" \
#       LDFLAGS="-lpbiscmock"
#
# n.b. mk_cmock accepts all mk_program parameters

##

DEPENDS="path compiler program"

### section build

### section configure

option()
{
    mk_option \
        OPTION="test-dir" \
        VAR="MK_UNITYTEST_DIR" \
        PARAM="dir" \
        DEFAULT="${MK_PREFIX}/tests" \
        HELP="Directory (relative to staging directory) where CMock/Unity tests are placed"
}

mk_check_cmock()
{
    mk_check_library pbiscmock
    mk_check_headers unity.h cmock.h

    if [ "${HAVE_LIB_PBISCMOCK}" != no -a "${HAVE_UNITY_H}" != no -a "${HAVE_CMOCK_H}" != no ]
    then
        HAVE_CMOCK=yes
    else
        HAVE_CMOCK=no
    fi

    mk_msg "Unity/CMock available: $HAVE_CMOCK"

    mk_declare -i HAVE_CMOCK
}

mk_have_cmock()
{
    [ "$HAVE_CMOCK" = "yes" ]
}

# build a Cmock/unity unit test
mk_cmock()
{
    mk_have_cmock || mk_fail "mk_cmock: CMock/unity unavailable"

    mk_push_vars SOURCES OBJECTS CPPFLAGS LDFLAGS HEADERS LIBDIRS INCLUDEDIRS LIBDEPS HEADERDEPS GROUPS DEPS
    mk_parse_params

    unset _CPPFLAGS _rsources _deps

    for _dir in ${INCLUDEDIRS}
    do
        _CPPFLAGS="$_CPPFLAGS -I${MK_SOURCE_DIR}${MK_SUBDIR}/${_dir} -I${MK_OBJECT_DIR}${MK_SUBDIR}/${_dir}"
    done

    for _header in ${HEADERDEPS}
    do
        _deps="$_deps '${MK_INCLUDEDIR}/${_header}'"
    done

    mk_program \
        PROGRAM="${PROGRAM}" \
        INSTALLDIR="${MK_UNITYTEST_DIR}${MK_SUBDIR}" \
        SOURCES="${SOURCES}" \
        OBJECTS="${OBJECTS}" \
        HEADERS="${HEADERS}" \
        CPPFLAGS="${CPPFLAGS}" \
        CFLAGS="${_cflags}" \
        LDFLAGS="${LDFLAGS}" \
        LIBDIRS="${LIBDIRS}" \
        LIBDEPS="${LIBDEPS}" \
        INCLUDEDIRS="${INCLUDEDIRS}" \
        HEADERDEPS="${HEADERDEPS}" \
        GROUPS="${GROUPS}" \
        DEPS="${DEPS}"

    # n.b. the list of tests is not used by the test runner but could be
    MK_CMOCK_UNITTESTS="${MK_CMOCK_UNITTESTS} $result"

    mk_pop_vars
}

configure()
{
    if [ "$MK_CROSS_COMPILING" = yes ]
    then
        mk_msg "cross compiling -- test cannot be run"
    else
        mk_check_cmock
    fi
}

make()
{
    if [ -n "$MK_CMOCK_UNITTESTS" ]
    then
        mk_target \
            TARGET="@run-tests" \
            DEPS="${MK_CMOCK_UNITTESTS}" \
            mk_run_script cmock "${MK_ROOT_DIR}/${MK_STAGE_DIR}${MK_LIBDIR}" "${MK_STAGE_DIR}${MK_UNITYTEST_DIR}"

        mk_add_phony_target "$result"

        mk_add_clean_target "@${MK_STAGE_DIR}${MK_UNITYTEST_DIR}"
    fi
}
