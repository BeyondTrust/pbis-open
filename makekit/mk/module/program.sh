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

mk_check_program()
{
    mk_push_vars PROGRAM FAIL
    mk_parse_params
    
    if [ -z "$PROGRAM" ]
    then
	PROGRAM="$1"
    fi
    
    _mk_define_name "$PROGRAM"
    _def="$result"
    _res=""
    
    if _mk_contains "$PROGRAM" "$MK_INTERNAL_PROGRAMS"
    then
	_res="${MK_RUN_BINDIR}/${PROGRAM}"
    else
	_IFS="$IFS"
	IFS=":"
	for __dir in ${MK_PATH} ${PATH}
	do
	    if [ -x "${__dir}/${PROGRAM}" ]
	    then
		_res="${__dir}/${PROGRAM}"
		break;
	    fi
	done
	IFS="$_IFS"
    fi
    
    mk_export "$_def=$_res"
    
    if [ -z "$_res" ]
    then
	mk_msg "program $PROGRAM: not found"
	if [ "$FAIL" = "yes" ]
	then
	    mk_fail "could not find program: $PROGRAM"
	fi
	mk_pop_vars
	return 1
    else
	mk_msg "program $PROGRAM: $_res"
	mk_pop_vars
	return 0
    fi
}
