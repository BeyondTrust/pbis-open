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

MK_MSG_DOMAIN="scrub"

SUBDIR="$1"
shift

if [ -z "$SUBDIR" ]
then
    mk_quote_list "$@"
    EXTRA_TARGETS="$result"
fi

mk_get_stage_targets "@$SUBDIR"
mk_unquote_list "$result $EXTRA_TARGETS"

for _target
do
    if [ -e "${_target#@}" -o -h "${_target#@}" ]
    then
        mk_msg "${_target#@$MK_STAGE_DIR}"
        mk_safe_rm "${_target#@}"
    fi
done

if [ -d "$MK_STAGE_DIR" ]
then
    find "${MK_STAGE_DIR}" -type d | sed '1!G;h;$!d' |
    while read -r _dir
    do
        if rmdir -- "$_dir" >/dev/null 2>&1
        then
            if [ "$_dir" = "$MK_STAGE_DIR" ]
            then
                mk_msg "${_dir}"
            else
                mk_msg "${_dir#$MK_STAGE_DIR}"
            fi
        fi
    done
fi