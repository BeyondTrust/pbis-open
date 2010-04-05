#!/bin/bash

if [ -z "${BUILD_ROOT}" ]; then
    echo "You need to source buildenv first" 1>&2
    exit 1
fi

_vc_have_svn()
{
    type svn >/dev/null 2>&1
}

_vc_have_git()
{
    type git >/dev/null 2>&1
}

_vc_is_svn()
{
    test -d "${BUILD_ROOT}/.svn"
}

_vc_is_git()
{
    test -d "${BUILD_ROOT}/.git"
}

_vc_use_svn()
{
    _vc_is_svn && _vc_have_svn
}

_vc_use_git()
{
    _vc_is_git && _vc_have_git
}

vc_is_available()
{
    _vc_use_svn || _vc_use_git
}

vc_have_local_changes()
{
    # inputs: paths passed in as $@
    # outputs: n/a
    # returns: 0 if local changes found, 1 otherwise

    if _vc_use_svn ; then
        local _local_changes=`svn status "$@" 2>/dev/null | grep -v '^\?'`
        if [ -n "${_local_changes}" ]; then
            return 0
        fi
    elif _vc_use_git ; then
        local path
        for path in "$@" ; do
            local _local_changes=`cd "${path}" 2>/dev/null && git ls-files -m 2>/dev/null`
            if [ -n "${_local_changes}" ]; then
                return 0
            fi
        done
    fi
    return 1
}

_vc_convert_local_string_time()
{
    # inputs: local date/time string of the form "Thu Apr 30 18:33:27 2009"
    # outputs: GMT date/time string of the form YYYYMMDDHHmmSS
    # returns: n/a

    if [ -n "$1" ]; then
        perl -w <<EOF
use strict;
use warnings;
use Time::Local;
if ('$1' =~ /^\\S+\\s+(\\S+)\\s+(\\d+)\\s+(\\d+):(\\d+):(\\d+)\s+(\\d+)$/)
{
    my \$month_text = \$1;
    my \$day = \$2;
    my \$hour = \$3;
    my \$minute = \$4;
    my \$second = \$5;
    my \$year = \$6;

    my \$month = undef;
    my \$i = 1;
    foreach my \$abbr (qw( Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec ))
    {
        if ( \$abbr eq \$month_text )
        {
            \$month = \$i;
            last;
        }
        \$i++;
    }
    die "Cannot find month for \$month_text\n" if not defined(\$month);
    my \$time = timelocal(\$second, \$minute, \$hour, \$day, \$month - 1, \$year);
    (\$second, \$minute, \$hour, \$day, \$month, \$year) = gmtime(\$time);
    \$month += 1;
    \$year += 1900;
    print sprintf("%04d%02d%02d%02d%02d%02d", \$year, \$month, \$day, \$hour, \$minute, \$second),"\n";
}
EOF
    fi
}

_vc_convert_unix_time()
{
    # inputs: Unix time
    # outputs: GMT date/time string of the form YYYYMMDDHHmmSS
    # returns: n/a

    if [ -n "$1" ]; then
        perl -w <<EOF
use strict;
use warnings;
my (\$second, \$minute, \$hour, \$day, \$month, \$year) = gmtime($1);
\$month += 1;
\$year += 1900;
print sprintf("%04d%02d%02d%02d%02d%02d", \$year, \$month, \$day, \$hour, \$minute, \$second),"\n";
EOF
    fi
}

vc_changed_revision_1()
{
    # inputs: path passed in as $1
    # outputs: revision number or path, or UNKNOWN
    # returns: n/a

    local _path="$1"
    local _rev=""

    if [ -n "${_path}" -a -e "${_path}" ]; then
        if _vc_use_svn ; then
            _rev=`svn info "${_path}" 2>/dev/null | grep 'Last Changed Rev:' | cut -d: -f2 | sed -e 's/^ 	*//g'`
        elif _vc_use_git ; then
            _rev=`git log -n1 --pretty=format:%ct "${_path}" 2>/dev/null | sed -e 's/[ :-]//g' -e 's/+.*//'`
            _rev=`_vc_convert_unix_time "${_rev}"`
        fi
        if [ -z "${_rev}" ]; then
            _rev="UNKNOWN"
        fi
    fi
    echo "${_rev}"
}

_vc_changed_revision()
{
    # inputs: paths passed in as $@
    # outputs: highest revision number or nothing on failure
    # returns: n/a

    local _rev=""
    local _recurse=""
    if [ -z "${OFFICIAL_BUILD_NUMBER}" ]; then
        _recurse="-R"
    fi
    if _vc_use_svn ; then
        svn info ${_recurse} "$@" 2>/dev/null | grep "^Last Changed Rev:" | awk '{print $4}' | sort -n | tail -1
    elif _vc_use_git ; then
        local _path=''
        for _path in "$@" ; do
            local _one_rev=`git log -n1 --pretty=format:%ct "${_path}" 2>/dev/null | sed -e 's/[ :-]//g' -e 's/+.*//'`
            _one_rev=`_vc_convert_unix_time "${_one_rev}"`
            _rev="${_rev} ${_one_rev}"
        done
        echo "${_rev}" | xargs -n1 | sort | tail -1
    elif [ "${BUILD_OS_TYPE}" = "linux" ]; then
	( IFS=$'\n'; for file in `find "$@" | grep -v '\.svn' | grep -v '\.git'`; do stat -c "%Y" "$file"; done ) | sort | tail -n 1
    elif [ "${BUILD_OS_TYPE}" = "freebsd" ]; then
	( IFS=$'\n'; for file in `find "$@" | grep -v '\.svn' | grep -v '\.git'`; do stat -f "%m" "$file"; done) | sort | tail -n 1
    fi
}

vc_changed_revision()
{
    # inputs: paths passed in as $@
    # outputs: highest revision number or nothing on failure
    # returns: 0 on success, 1 on failure

    local _rev=`_vc_changed_revision "$@"`
    test -n "${_rev}" || return 1
    echo "${_rev}"
    return 0
}
