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
# doc.sh -- generates mkdoc xml from modules
#
##

. "${MK_HOME}/mk.sh" || exit 1

MK_SEARCH_DIRS="${MK_HOME}"
TITLE="MakeKit Reference"
INDEX="false"
HEADER="false"
MODE="file"

emit_header()
{
    mk_quote_c_string "$TITLE"
    printf '<reference title=%s>\n' "$result"
}

emit_footer()
{
    printf '</reference>\n'
}

process_file()
{
    if $INDEX
    then
        awk -f "$MK_HOME/doc.awk" -v title="$TITLE" "$@" |
        grep "^<function" |
        sed -e 's/<function name="//' -e 's/".*$//' || mk_fail "awk failed"

        awk -f "$MK_HOME/doc.awk" -v title="$TITLE" "$@" |
        grep "^<variable" |
        sed -e 's/<variable name="//' -e 's/".*$//' || mk_fail "awk failed"
    else
        awk -f "$MK_HOME/doc.awk" -v title="$TITLE" "$@" || mk_fail "awk failed"
    fi
}

process_module()
{
    _mk_find_resource "module/$1.sh" || mk_fail "could not find module $1"
    process_file "$result"
}

process_docbook()
{
    if ! $INDEX
    then
        mk_quote_c_string "$1"
        printf '<include format="docbook" name="%s" file=%s/>\n' "${1##*/}" "$result"
    fi
}

while [ $# -gt 0 ]
do
    case "$1" in
        --index) INDEX="true"; shift;;
        --title) TITLE="$2"; shift 2;;
        --module) MODE=module; shift;;
        --file) MODE=file; shift;;
        --docbook) MODE=docbook; shift;;
        *)
            if ! $HEADER && ! $INDEX
            then
                emit_header
                HEADER=true
            fi

            case "$MODE" in
                file) process_file "$1";;
                module) process_module "$1";;
                docbook) process_docbook "$1";;
            esac
            shift
            ;;
    esac
done

if ! $INDEX
then
    emit_footer
fi

