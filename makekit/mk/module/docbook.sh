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
# docbook.sh -- support for building DocBook documentation
#
# FIXME: move to contrib module area?
#
##

DEPENDS="program core"

### section configure

option()
{
    _default_docbook_dir="<none>"

    for candidate in \
        /usr/share/xml/docbook/stylesheet/docbook-xsl
    do
        if [ -d "$candidate" ]
        then
            _default_docbook_dir="$candidate"
            break
        fi
    done
    
    mk_option \
        OPTION=docbook-xsl-dir \
        PARAM="path" \
        VAR="MK_DOCBOOK_XSL_DIR" \
        DEFAULT="$_default_docbook_dir" \
        HELP="Location of DocBook XSL stylesheets"
}

configure()
{
    mk_msg "xsl dir: $MK_DOCBOOK_XSL_DIR"
    mk_check_program "xsltproc"

    if [ -d "$MK_DOCBOOK_XSL_DIR" -a -n "$XSLTPROC" ]
    then
        MK_HAVE_DOCBOOK=yes
    else
        MK_HAVE_DOCBOOK=no
    fi

    mk_msg "docbook enabled: $MK_HAVE_DOCBOOK"
    mk_export MK_HAVE_DOCBOOK MK_DOCBOOK_XSL_DIR
}

mk_have_docbook()
{
    [ "$MK_HAVE_DOCBOOK" = "yes" ]
}

mk_docbook_html()
{
    mk_push_vars \
        STYLESHEET="@$MK_DOCBOOK_XSL_DIR/xhtml/profile-chunk.xsl" \
        IMAGES="@$MK_DOCBOOK_XSL_DIR/images" \
        INSTALLDIR="${MK_HTMLDIR}" \
        SOURCE \
        DEPS

    mk_parse_params

    mk_target \
        TARGET="${INSTALLDIR}" \
        DEPS="$DEPS $SOURCE $STYLESHEET" \
        _mk_docbook '$@/' "&$SOURCE" "&$STYLESHEET"

    mk_add_all_target "$result"

    # Install CSS file
    [ -n "$CSS" ] && mk_stage SOURCE="$CSS" DESTDIR="${INSTALLDIR}"

    # Install image files
    [ -n "$IMAGES" ] && mk_stage SOURCE="$IMAGES" DEST="${INSTALLDIR}/images"

    mk_pop_vars
}

mk_docbook_man()
{
    mk_push_vars \
        STYLESHEET="@$MK_DOCBOOK_XSL_DIR/manpages/profile-docbook.xsl" \
        INSTALLDIR="${MK_MANDIR}" \
        SOURCE \
        MANPAGES \
        DEPS
    mk_parse_params

    mk_target \
        TARGET="${SOURCE}.docbook-man" \
        DEPS="$DEPS $SOURCE" \
        _mk_docbook '$@/' "&$SOURCE" "&$STYLESHEET"
    man_output="$result"

    mk_unquote_list "$MANPAGES"
    for manfile
    do
        section="${manfile##*.}"
        __tail="${section#?}"
        section="${section%$__tail}"
        
        mk_target \
            TARGET="$man_output/$manfile" \
            DEPS="$man_output"

        mk_stage \
            SOURCE="$man_output/$manfile" \
            DESTDIR="$INSTALLDIR/man${section}"
    done

    mk_pop_vars
}

### section build

_mk_docbook()
{
    MK_MSG_DOMAIN="xsltproc"

    mk_msg "${1#${MK_STAGE_DIR}}"
    mk_mkdir "${1%/*}"
    mk_run_or_fail \
        "${XSLTPROC}" \
        --xinclude \
        --output "$1" \
        "$3" \
        "$2"
    mk_run_or_fail touch "$1"
}
