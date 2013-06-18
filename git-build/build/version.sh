#!/bin/bash

if [ -z "${BUILD_ROOT}" ]; then
    echo "You need to source buildenv first" 1>&2
    exit 1
fi

MAIN_NAME=likewise
MAIN_VERSION=5.0.0
MAIN_RELEASE=1

MAIN_PREFIX_DIR="${BUILD_PREFIX}"

function __get_svn_last_changed_revision
{
    local _path="$1"
    local _rev=""

    local _revFile="${BUILD_ROOT}/src/linux/REVISION"

    if [[ -n "${_path}" && -e "${_path}" ]]; then
        type -t svn 2>&1 > /dev/null
        if [ $? -eq 0 ]; then
            _rev="`svn info ${_path} | grep 'Last Changed Rev:' | cut -d: -f2 | sed -e 's/^ 	*//g'`"
        elif [ -f "${_revFile}" ]; then 
            _rev="`cat ${_revFile}`"
        fi
        if [ -z "${_rev}" ]; then
            _rev="UNKNOWN"
        fi
    fi
    echo "${_rev}"
}

SVN_REVISION=`__get_svn_last_changed_revision "${BUILD_ROOT}"`
SVN_REVISION_SRC=`__get_svn_last_changed_revision "${BUILD_ROOT}"`
