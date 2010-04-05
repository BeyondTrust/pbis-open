#!/bin/bash

if [ -z "${BUILD_ROOT}" ]; then
    echo "You need to source buildenv first" 1>&2
    exit 1
fi

source "${BUILD_ROOT}/src/linux/build/lib/vc-helper.sh"

MAIN_NAME=likewise
MAIN_VERSION=5.5.0
MAIN_RELEASE=1

MAIN_PREFIX_DIR="${BUILD_PREFIX}"

SVN_REVISION=`vc_changed_revision_1 "${BUILD_ROOT}"`

export MAIN_NAME MAIN_VERSION MAIN_RELEASE MAIN_PREFIX_DIR SVN_REVISION
