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
# package.sh -- ad-hoc building of binary packages
#
# This module is the core of a semi-abstract interface
# for building binary packages from the staging area.
#
# Most packaging systems (rpm, debian, etc) use a
# spec file or rules script to invoke the build system
# for the source code and then package up the resulting
# binaries with some metadata.  This module inverts
# that arrangement by invoking the package build system
# from the source build system.  This has a few advantages:
#
# - Easy incremental rebuilding of packages during development
# - The source can be compiled once but packaged several
#   different ways or in different formats
#
# The disadvantages:
#
# - The packages aren't easily reproducible (no SRPMS, etc)
#
# Only some of the details of packaging, such as managing file
# lists, are abstracted away.  The user must still provide
# snippets of spec files, install scripts, etc. on a per-format
# basis.  This allows some build logic to be shared while
# still providing full control over each format's idiosyncracies.
# 
# This module only contains core logic.  The user must use
# package-deb, package-rpm, etc. as appropriate.
# 
##

# API overview
#
# mk_{PACKAGE_TYPE}_do
#   mk_package_patterns
#   mk_package_targets
#   mk_package_dirs
# mk_{PACKAGE_TYPE}_done

### section configure

DEPENDS="core"

#<
# @function mk_package_targets
# @brief Include specified targets in current package
# @usage targets...
#
# Includes the specified targets in the current package
# or subpackage.  The targets must be specified in
# fully-qualified form.
#>

#<
# @function mk_package_dirs
# @brief Specify inclusion of directory in current package
# @usage dirs...
#
# Explicitly adds a set of directories to the current package.
# This is useful for including empty directories or controlling
# directory attributes.
#>

#<
# @brief Package targets matching a set of patterns
# @usage patterns...
# @option SUBDIRS=subdir_list A list of subdirectories
# relative to the current MakeKitBuild file from which
# to select targets.  Targets will be selected from all
# of the specified directories and their transitive
# subdirectories.  A directory in the list may begin
# with <lit>@</lit> to indicate that it is relative
# to the top source directory of the project.  Defaults
# to the entire project.
#
# Packages all targets in the staging area matching any
# of the file patterns in <param>patterns</param>.
# The selected targets may be limited to those within
# certain subsets of the current project by using
# the <param>subdir_list</param> option.
#>
mk_package_patterns()
{
    mk_push_vars SUBDIRS="@" SELECT PATTERN TARGET
    mk_parse_params

    if [ "$#" = 0 ]
    then
        SELECT="*"
    else
        for PATTERN
        do
            case "$PATTERN" in
                /*)
                    mk_quote "@${MK_STAGE_DIR}${PATTERN}"
                    SELECT="$SELECT $result"
                    ;;
                *)
                    mk_quote "$PATTERN"
                    SELECT="$SELECT $result"
                    ;;
            esac
        done
    fi

    mk_unquote_list "$SUBDIRS"
    mk_get_stage_targets SELECT="$SELECT" "$@"
    mk_unquote_list "$result"
    mk_package_targets "$@"
    
    mk_pop_vars
}

mk_add_package_target()
{
    mk_push_vars result
    mk_resolve_target "$1"
    mk_quote "$result"
    _MK_PACKAGE_TARGETS="$_MK_PACKAGE_TARGETS $result"
    mk_pop_vars
}

option()
{
    mk_option \
        OPTION="package-dir" \
        VAR="MK_PACKAGE_DIR" \
        PARAM="path" \
        DEFAULT="package" \
        HELP="Subdirectory for built packages"

    mk_option \
        OPTION="package-debug" \
        VAR="MK_PACKAGE_DEBUG" \
        PARAM="yes|no" \
        DEFAULT="no" \
        HELP="Specify \"-debug\" suffix in package name"
}

make()
{
    mk_target \
        TARGET="@package" \
        DEPS="$_MK_PACKAGE_TARGETS"

    mk_add_phony_target "$result"
    
    mk_add_scrub_target "@package"
}
