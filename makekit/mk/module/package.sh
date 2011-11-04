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
#   mk_package_files
#   mk_package_dirs
#   mk_subpackage_do
#     mk_package_files
#     mk_package_dirs
#   mk_subpackage_done
# mk_{PACKAGE_TYPE}_done

### section configure

DEPENDS="core"

option()
{
    mk_option \
        OPTION="package-dir" \
        VAR="MK_PACKAGE_DIR" \
        PARAM="path" \
        DEFAULT="package" \
        HELP="Subdirectory for built packages"
}

configure()
{
    mk_export MK_PACKAGE_DIR

    mk_add_scrub_target "@package"
}
