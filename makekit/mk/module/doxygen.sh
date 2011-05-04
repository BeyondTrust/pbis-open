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
# doxygen.sh -- support for building doxygen documentation
#
##

DEPENDS="core"

### section configure

#<
# @brief Test if doxygen was found
# @usage
#
# Returns <lit>0</lit> (logical true) if doxygen was successfully
# found by <funcref>mk_check_doxygen</funcref>, or <lit>1</lit>
# (logical false) otherwise.
#>
mk_have_doxygen()
{
    [ -n "$DOXYGEN" ]
}

#<
# @brief Generate html documentation
# @usage INPUT=input_list
# @option INPUT=input_list a list of files that should be
# processed by <cmd>doxygen</cmd>
# @option HEADERDIRS=dir_list a list of source directories
# relative to the current MakeKitBuild where public header
# files are installed (e.g. by <funcref>mk_headers</funcref>).
# The headers will be added to the input list automatically.
# @option DOXYFILE a Doxygen configuration file to use. If
# not specified, defaults to <lit>Doxyfile</lit>.  The file
# should not specify any options controlling input or output
# files as they will be filled in automatically.
# @option INSTALLDIR=dir where to install the html documentation.
# Defaults to <lit>$MK_HTMLDIR/doxygen</lit>.
#
# Processes the specified input file and headers with doxygen
# and outputs documentation in html format.  You must provide
# a Doxyfile in the current directory (or override the path
# using <lit>DOXYFILE=</lit><param>doxyfile</param>) to control
# any customizable Doxygen settings.
#
# To use this function, you must use <funcref>mk_check_doxygen</funcref>
# in a <lit>configure</lit> section of your project, and it must succeed.
# You can test whether Doxygen was found with <funcref>mk_have_doxygen</funcref>.
# This function will abort if Doxygen is unavailable.
#>
mk_doxygen_html()
{
    mk_push_vars \
        INSTALLDIR="${MK_HTMLDIR}/doxygen" \
        DOXYFILE="Doxyfile" \
        HEADERDIRS \
        EXAMPLES \
        HEADERS \
        INPUT
    mk_parse_params
    
    mk_have_doxygen || mk_fail "mk_doxygen_html: doxygen is unavailable"

    mk_unquote_list "$HEADERDIRS"
    mk_get_stage_targets SELECT="*.h *.hpp" "$@"
    HEADERS="$result"

    mk_resolve_files "$EXAMPLES"
    EXAMPLES="$result"

    mk_resolve_targets "$INPUT"
    INPUT="$result"

    mk_target \
        TARGET="${INSTALLDIR}" \
        DEPS="$HEADERS $INPUT $DOXYFILE" \
        _mk_doxygen_html %EXAMPLES '$@' "&$DOXYFILE" "*$HEADERS" "*$INPUT"
    
    mk_pop_vars
}

#<
# @brief Check for Doxygen on the build system
# @usage
#
# Checks for the availability of Doxygen on the build system.
# The result can be tested with <funcref>mk_have_doxygen</funcref>.
# If successful, you may then use functions such as
# <funcref>mk_doxygen_html</funcref> to build Doxygen documentation
# as part of your project.
#>
mk_check_doxygen()
{
    mk_check_program doxygen
}

### section build

_mk_doxygen_html()
{
    # $1 = installdir
    # $2 = Doxyfile
    # ... = sources
    mk_push_vars EXAMPLES
    mk_parse_params

    mk_msg_domain doxygen

    mk_msg "${1#$MK_STAGE_DIR}"

    mk_mkdir "$1"
    
    {
        cat "$2"
        echo "GENERATE_LATEX = no"
        echo "GENERATE_HTML = yes"
        echo "OUTPUT_DIRECTORY ="
        echo "HTML_OUTPUT = $1"
        echo "FULL_PATH_NAMES = yes"
        echo "STRIP_FROM_PATH = ${MK_STAGE_DIR}${MK_INCLUDEDIR}"
        echo "STRIP_FROM_INC_PATH = ${MK_STAGE_DIR}${MK_INCLUDEDIR}"
        echo "INPUT = "
        echo "EXAMPLE_PATH = "
        shift 2
        for header
        do
            echo "INPUT += ${header#@}"
        done

        mk_unquote_list "$EXAMPLES"
        for example
        do
            echo "EXAMPLE_PATH += $example"
        done
    } | doxygen - || mk_fail "failed to run doxygen"

    mk_pop_vars
}
