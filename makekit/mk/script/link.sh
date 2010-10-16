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

MK_MSG_DOMAIN="link"

version_pre()
{
    case "$MODE" in
	library)
	    if [ -z "$VERSION" ]
	    then
		VERSION="0.0.0"
	    fi
	    ;;
	program)
	    return 0
	    ;;
    esac
    
    if [ "$VERSION" != "no" ]
    then
	_rest="${VERSION}."
	MAJOR="${_rest%%.*}"
	_rest="${_rest#*.}"
	MINOR="${_rest%%.*}"
	_rest="${_rest#*.}"
	MICRO="${_rest%%.*}"
    fi
    
    if [ -n "$MAJOR" ]
    then
	SONAME="${object##*/}.$MAJOR"
	LINK1="${object}"
	object="${object}.$MAJOR"
    fi
    
    if [ -n "$MINOR" ]
    then
	LINK2="${object}"
	object="${object}.$MINOR"
    fi
    
    if [ -n "$MICRO" ]
    then
	LINK3="${object}"
	object="${object}.$MICRO"
    fi
    
    if [ -n "$SONAME" ]
    then
	COMBINED_LDFLAGS="$COMBINED_LDFLAGS -Wl,-h,$SONAME"
    fi
}

version_post()
{
    _target="${object}"

    for _link in "$LINK3" "$LINK2" "$LINK1"
    do
	if [ -n "$_link" ]
	then
	    mk_run_or_fail ln -sf "${_target##*/}" "${_link}"
	    _target="$_link"
	fi
    done
}

combine_libtool_flags()
{
    for _lib in ${COMBINED_LIBDEPS}
    do
	for _path in ${COMBINED_LDFLAGS} ${MK_LDFLAGS} -L/usr/lib -L/lib
	do
	    case "$_path" in
		"-L"*)
		    if [ -e "${_path#-L}/lib${_lib}.la" ]
		    then
			unset dependency_libs
			mk_safe_source "${_path#-L}/lib${_lib}.la" || mk_fail "could not read libtool archive"
			for _dep in ${dependency_libs}
			do
			    case "$_dep" in
				"${MK_LIBDIR}"/*.la)
				    _dep="${_dep##*/}"
				    _dep="${_dep#lib}"
				    _mk_contains "${_dep%.la}" ${COMBINED_LIBDEPS} ||
				    COMBINED_LIBDEPS="${COMBINED_LIBDEPS} ${_dep%.la}" 
				    ;;
				"-l"*)
				    _mk_contains "${_dep#-l}" ${COMBINED_LIBDEPS} ||
				    COMBINED_LIBDEPS="${COMBINED_LIBDEPS} ${_dep#-l}"
				    ;;
				"-L"*)
				    _mk_contains "${_dep}" ${COMBINED_LDFLAGS} ||
				    COMBINED_LDFLAGS="$COMBINED_LDFLAGS $_dep"
				    ;;
			    esac
			done
			break
		    fi
		    ;;
	    esac
	done
    done
}

create_libtool_archive()
{
    # Create a fake .la file that can be used by combine_libtool_flags
    # This should be expanded upon for full compatibility with libtool
    mk_msg_verbose "${object%${EXT}}.la"
    
    {
	mk_quote "-L${RPATH_LIBDIR} $_LIBS"
	echo "# Created by MakeKit"
	echo "dependency_libs=$result"
    } > "${object%${EXT}}.la" || mk_fail "could not write ${object%${EXT}}.la"
}

object="$1"
shift 1


if [ "${MK_SYSTEM%/*}" = "build" ]
then
    LINK_LIBDIR="$MK_RUN_LIBDIR"
    RPATH_LIBDIR="$MK_ROOT_DIR/$MK_RUN_LIBDIR"
else
    RPATH_LIBDIR="$MK_LIBDIR"
    mk_resolve_file "$MK_LIBDIR"
    LINK_LIBDIR="$result"
fi

COMBINED_LIBDEPS="$LIBDEPS"
COMBINED_LDFLAGS="$LDFLAGS -L${LINK_LIBDIR}"
COMBINED_LIBDIRS="$LIBDIRS"

# Group suffix
_gsuffix="-${MK_CANONICAL_SYSTEM%/*}-${MK_CANONICAL_SYSTEM#*/}.og"

for _group in ${GROUPS}
do
    unset OBJECTS LIBDEPS LIBDIRS LDFLAGS
    mk_safe_source "${MK_OBJECT_DIR}${MK_SUBDIR}/$_group${_gsuffix}" || mk_fail "Could not read group $_group"

    GROUP_OBJECTS="$GROUP_OBJECTS ${OBJECTS}"
    COMBINED_LIBDEPS="$COMBINED_LIBDEPS $LIBDEPS"
    COMBINED_LIBDIRS="$COMBINED_LIBDIRS $LIBDIRS"
    COMBINED_LDFLAGS="$COMBINED_LDFLAGS $LDFLAGS"
done

case "${MK_OS}" in
    linux|freebsd)
	COMBINED_LDFLAGS="$COMBINED_LDFLAGS -Wl,-rpath,${RPATH_LIBDIR} -Wl,-rpath-link,${LINK_LIBDIR}"
	;;
    solaris)
	COMBINED_LDFLAGS="$COMBINED_LDFLAGS -R${RPATH_LIBDIR}"
	
	if [ "$MODE" = "library" ]
	then
	    COMBINED_LDFLAGS="$COMBINED_LDFLAGS -Wl,-z,defs -Wl,-z,text"
	    COMBINED_LIBDEPS="$COMBINED_LIBDEPS c"
	fi

	# The solaris linker is anal retentive about implicit shared library dependencies,
	# so use available libtool .la files to add implicit dependencies to the link command
	combine_libtool_flags
	;;
esac

for lib in ${COMBINED_LIBDEPS}
do
    _LIBS="$_LIBS -l${lib}"
done

[ "${object%/*}" != "${object}" ] && mk_mkdir "${object%/*}"

case "$MODE" in
    library|dlo)
	create_libtool_archive
	;;
esac

version_pre

mk_msg "${object#${MK_STAGE_DIR}} ($MK_CANONICAL_SYSTEM)"

case "$MODE" in
    library)
	mk_run_or_fail ${MK_CC} -shared -o "$object" "$@" ${GROUP_OBJECTS} ${COMBINED_LDFLAGS} ${MK_LDFLAGS} -fPIC ${_LIBS}
	;;
    dlo)
	mk_run_or_fail ${MK_CC} -shared -o "$object" "$@" ${GROUP_OBJECTS} ${COMBINED_LDFLAGS} ${MK_LDFLAGS} -fPIC ${_LIBS}
	;;
    program)
	mk_run_or_fail ${MK_CC} -o "$object" "$@" ${GROUP_OBJECTS} ${COMBINED_LDFLAGS} ${MK_LDFLAGS} ${_LIBS}
	;;
esac

version_post
