#!/bin/bash

function get_tar_utility
{
    if [ "${BUILD_OS_TYPE}" = "solaris" ]; then
        echo /opt/csw/bin/gtar
    else
        echo tar
    fi
}

function get_archive_suffix
{
    echo ".tar.bz2"
}

function uncompress_archive
{
    local file="$1"
    local tar=`get_tar_utility`
    check_arg file "${file}"
    bunzip2 < "${file}" | "${tar}" xf -
}

function compress_archive
{
    local sourceDir="$1"
    local targetFile="$2"
    check_arg sourceDir "${sourceDir}"
    check_arg targetFile "${targetFile}"
    local tar=`get_tar_utility`
    "${tar}" cf - "${sourceDir}" | bzip2 > "${targetFile}"
}
