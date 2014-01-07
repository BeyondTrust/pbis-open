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
# init.sh -- installs MakeKit into a project directory
#
##

. "${MK_HOME}/mk.sh" || exit 1

init_usage()
{
    echo "makekit init -- install MakeKit files into current directory"
    echo ""
    echo "Usage: makekit init [ options ]"
}

init_help()
{
    init_usage

    echo ""
    echo "Options:"
    echo ""
    echo "  -h,--help         Show this help"
    echo "  -s                Symlink files instead of copying them"
    echo "  -f                Overwrite existing files"
}

init_parse_params()
{
    while [ "$#" -gt 0 ]
    do
        _param="$1"
        shift
        case "$_param" in
            --help|-h)
                OPTION_HELP=yes
                ;;
            -s)
                OPTION_SYMLINK=yes
                ;;
            -f)
                OPTION_FORCE=yes
                ;;
            *)
                init_usage
                exit 1
                ;;
        esac
    done
}

do_install()
{
    if [ "$OPTION_FORCE" = "yes" ]
    then
        if [ -e "$2" ]
        then
            rm -rf "$2"
        fi
    fi
    
    [ -e "$2" ] && return 0

    if [ "$OPTION_SYMLINK" = "yes" ]
    then
        ln -s "$1" "$2" || mk_fail "could not symlink $1 to $2"
    else
        cp -r "$1" "$2" || mk_fail "could not copy $1 to $2"
    fi
}

mk_msg_domain init

init_parse_params "$@"

if [ "$OPTION_HELP" = "yes" ]
then
    init_help
    exit 0
fi

do_install "${MK_HOME}" "mk"
do_install "${MK_HOME}/makekit" "configure"