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

object="$1"
shift 1

_ALL_OBJECTS="$*"
_ALL_LIBDEPS="$LIBDEPS"
_ALL_LIBDIRS="$LIBDIRS"
_ALL_LDFLAGS="$LDFLAGS"

MK_MSG_DOMAIN="group"

# Group suffix
_gsuffix=".${MK_CANONICAL_SYSTEM%/*}.${MK_CANONICAL_SYSTEM#*/}.og"
_name="${object#${MK_OBJECT_DIR}/}"
_name="${_name%$_gsuffix}"

mk_msg "$_name ($MK_CANONICAL_SYSTEM)"

for _group in ${GROUPDEPS}
do
    mk_safe_source "${MK_OBJECT_DIR}${MK_SUBDIR}/${_group}${_gsuffix}" || mk_fail "Could not read group: $_group"
    _ALL_OBJECTS="$_ALL_OBJECTS $OBJECTS"
    _ALL_LIBDEPS="$_ALL_LIBDEPS $LIBDEPS"
    _ALL_LIBDIRS="$_ALL_LIBDIRS $LIBDIRS"
    _ALL_LDFLAGS="$_ALL_LDFLAGS $LDFLAGS"
done

mk_mkdir "`dirname "$object"`"

{
    echo "COMPILER='$COMPILER'"
    mk_quote "${_ALL_OBJECTS# }"
    echo "OBJECTS=$result"
    mk_quote "${_ALL_LIBDEPS# }"
    echo "LIBDEPS=$result"
    mk_quote "${_ALL_LIBDIRS# }"
    echo "LIBDIRS=$result"
    mk_quote "${_ALL_LDFLAGS# }"
    echo "LDFLAGS=$result"
} > "${object}" || mk_fail "Could not write group: ${object}"
touch "${object}"