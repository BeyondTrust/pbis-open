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
# program.sh -- search for runnable programs
#
# FIXME: move to core.sh?
#
##


### section configure

#<
# @brief Check for program on build system
# @usage PROGRAM=name
# @usage VAR=varname candidates...
# @option FAIL=yes|no If set to yes, fails configuration
# if the program is not found.  Defaults to no.
#
# Checks for an available program among a list of
# one or more candidates and sets a variable to
# the first one found.
#
# When <param>varname</param> is unspecified, it is
# derived automatically from <param>name</param>.
#>
mk_check_program()
{
    mk_push_vars VAR PROGRAM FAIL
    mk_parse_params
    
    if [ -z "$PROGRAM" ]
    then
        PROGRAM="$1"
        shift
    fi

    if [ -z "$VAR" ]
    then
        _mk_define_name "$PROGRAM"
        VAR="$result"
    fi

    set -- "$PROGRAM" "$@"

    mk_get "$VAR"
    [ -n "$result" ] && set -- "$result" "$@"
    
    _res=""

    for _cand
    do
        mk_msg_checking "program ${_cand##*/}"
        if _mk_contains "$_cand" "$MK_INTERNAL_PROGRAMS"
        then
            mk_msg_result "(internal)"
            _res="${MK_RUN_BINDIR}/${_cand}"
            break
        elif [ -x "$_cand" ]
        then
            _res="$_cand"
            mk_msg_result "$_cand"
            break
        else
            _IFS="$IFS"
            IFS=":"
            for __dir in ${MK_PATH} ${PATH}
            do
                if [ -x "${__dir}/${_cand}" ]
                then
                    _res="${__dir}/${_cand}"
                    mk_msg_result "$_res"
                    break
                fi
            done
            IFS="$_IFS"
        fi
        [ -n "$_res" ] && break
        mk_msg_result "no"
    done

    if [ -z "$_res" -a "$FAIL" = "yes" ]
    then
        mk_fail "could not find program: $PROGRAM"
    fi
   
    mk_declare -e "$VAR=$_res"
    
    mk_pop_vars
    [ -n "$_res" ]
}
