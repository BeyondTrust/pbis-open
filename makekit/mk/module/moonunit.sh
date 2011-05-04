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
# moonunit.sh -- support for building MoonUnit unit test libraries
#
# FIXME: move to contrib module area?
#
##

DEPENDS="path compiler program"

### section build

_mk_invoke_moonunit_stub()
{
    mk_push_vars CPPFLAGS
    mk_parse_params

    MK_MSG_DOMAIN="moonunit-stub"
    __output="$1"
    shift

    mk_msg "${__output#${MK_OBJECT_DIR}/}"

    if ! ${MOONUNIT_STUB} \
        CPP="$MK_CC -E" \
        CXXCPP="$MK_CXX -E" \
        CPPFLAGS="$MK_CPPFLAGS $CPPFLAGS -I${MK_STAGE_DIR}${MK_INCLUDEDIR}" \
        -o "$__output" \
        "$@"
    then
        rm -f "$__output"
        mk_fail "moonunit-stub failed"
    fi

    mk_pop_vars
}

### section configure

mk_moonunit()
{
    mk_have_moonunit || mk_fail "mk_moonunit: moonunit unavailable"

    mk_push_vars DLO SOURCES CPPFLAGS CFLAGS LDFLAGS HEADERS LIBDIRS INCLUDEDIRS LIBDEPS HEADERDEPS GROUPS DEPS
    mk_parse_params

    unset _CPPFLAGS _rsources _deps

    case "$DLO" in
        *)
            _stub="${DLO}-stub.c"
            ;;
    esac

    for _dir in ${INCLUDEDIRS}
    do
        _CPPFLAGS="$_CPPFLAGS -I${MK_SOURCE_DIR}${MK_SUBDIR}/${_dir} -I${MK_OBJECT_DIR}${MK_SUBDIR}/${_dir}"
    done

    for _header in ${HEADERDEPS}
    do
        _deps="$_deps '${MK_INCLUDEDIR}/${_header}'"
    done

    mk_target \
        TARGET="$_stub" \
        DEPS="$SOURCES $_deps" \
        _mk_invoke_moonunit_stub CPPFLAGS="$_CPPFLAGS $CPPFLAGS" '$@' "&$SOURCES"
    
    SOURCES="$SOURCES $_stub"

    mk_dlo \
        INSTALLDIR="@$MK_MOONUNIT_DIR" \
        DLO="$DLO" \
        SOURCES="$SOURCES" \
        HEADERS="$HEADERS" \
        CPPFLAGS="$CPPFLAGS" \
        CFLAGS="$CFLAGS" \
        LDFLAGS="$LDFLAGS" \
        LIBDIRS="$LIBDIRS" \
        INCLUDEDIRS="$INCLUDEDIRS" \
        LIBDEPS="$LIBDEPS moonunit" \
        HEADERDEPS="$HEADERDEPS" \
        GROUPS="$GROUPS" \
        DEPS="$DEPS"

    MK_MOONUNIT_TESTS="$MK_MOONUNIT_TESTS $result"

    mk_pop_vars
}

option()
{
    mk_option \
        OPTION="moonunit-dir" \
        VAR="MK_MOONUNIT_DIR" \
        PARAM="dir" \
        DEFAULT="mu" \
        HELP="Directory where MoonUnit tests are placed"
}

mk_check_moonunit()
{
    mk_check_program moonunit-stub
    mk_check_headers moonunit/moonunit.h
    mk_check_libraries moonunit
    
    if [ -n "$MOONUNIT_STUB" -a "$HAVE_MOONUNIT_MOONUNIT_H" != no -a "$HAVE_LIB_MOONUNIT" != no ]
    then
        HAVE_MOONUNIT=yes
    else
        HAVE_MOONUNIT=no
    fi
    
    mk_msg "moonunit available: $HAVE_MOONUNIT"

    mk_declare -i HAVE_MOONUNIT
}

mk_have_moonunit()
{
    [ "$HAVE_MOONUNIT" = "yes" ]
}

configure()
{
    if [ "$MK_CROSS_COMPILING" = yes ]
    then
        mk_msg "cross compiling -- tests cannot be run"
    else
        mk_check_program moonunit
    fi
}

make()
{
    if [ -n "$MOONUNIT" -a -n "$MK_MOONUNIT_TESTS" -a "$MK_CROSS_COMPILING" = no ]
    then
        mk_target \
            TARGET="@test" \
            DEPS="${MK_MOONUNIT_TESTS}" \
            mk_run_script moonunit "*${MK_MOONUNIT_TESTS}"

        mk_add_phony_target "$result"

        mk_add_clean_target "@${MK_MOONUNIT_DIR}"
    fi
}