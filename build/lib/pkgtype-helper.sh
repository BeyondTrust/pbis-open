#!/bin/bash

function pkgtype_get_all_pkgtypes
{
    echo "rpm deb pkg mac dmg depot bff freebsd"
}

function pkgtype_check_valid
{
    local _cur
    for _cur in `pkgtype_get_all_pkgtypes` ; do
        if [ "$1" = "${_cur}" ]; then
            return 0
        fi
    done
    return 1
}

function pkgtype_get_convert_pkgtype
{
    local _pkgtype="$1"

    check_arg pkgtype "${_pkgtype}"

    local _result="${_pkgtype}"
    case "${_pkgtype}" in
        deb)
            _result=rpm2deb
            ;;
#        lpp)
#            _result=rpm2lpp
#            ;;
    esac
    echo "${_result}"
    return 0
}

function pkgtype_get_current_os_pkgtype_list
{
    local _result=""
    case "${BUILD_OS_TYPE}" in
        linux)
	    if [ -f /etc/debian_version ]; then
		_result=deb
            else
                _result=rpm
            fi
            ;;
	freebsd)
	    _result=freebsd
	    ;;
        solaris)
            _result=pkg
            ;;
        aix)
            _result=bff
            ;;
        darwin)
            _result=mac
            ;;
        hpux)
            _result=depot
            ;;
        *)
            warn "Unsupported platform: ${BUILD_OS_TYPE}"
            exit 1
            ;;
    esac
    echo "${_result}"
    return 0
}

function pkgtype_get_current_os_pkgtype_allow_list
{
    local _result=""
    case "${BUILD_OS_TYPE}" in
        linux)
            _result="rpm deb"
            ;;
        solaris)
            _result=pkg
            ;;
        aix)
            _result="bff"
            ;;
        darwin)
            _result="mac dmg"
            ;;
        hpux)
            _result="depot"
            ;;
	freebsd)
	    _result="freebsd"
	    ;;
        *)
            warn "Unsupported platform: ${BUILD_OS_TYPE}"
            exit 1
            ;;
    esac
    echo "${_result}"
    return 0
}

function pkgtype_get_current_os_arch_list
{
    local _result=""
    case "${BUILD_OS_TYPE}" in
        linux)
            _result="i386 x86_64"
            ;;
        solaris)
            _result="i386 sparc"
            ;;
        aix)
            _result="powerpc"
            ;;
        darwin)
            _result="i386 powerpc x86_64"
            ;;
        hpux)
            _result="hppa20 ia64"
            ;;
	freebsd)
	    _result="i386"
	    ;;
        *)
            warn "Unsupported platform: ${BUILD_OS_TYPE}"
            exit 1
            ;;
    esac
    echo "${_result}"
    return 0
}

function pkgtype_convert_pkgtype_and_arch_to_larch
{
    local _pkgtype="$1"
    local _arch="$2"

    check_arg pkgtype "${_pkgtype}"
    check_arg pkgtype "${_arch}"

    local _result="${_arch}"
    case "${_pkgtype}" in
        deb)
            if [ "${_arch}" = "x86_64" ]; then
                _result=amd64
            fi
            ;;
    esac
    echo "${_result}"
    return 0
}

function pkgtype_get_current_os_pkgtype
{
    local _result
    local _list
    _list=`pkgtype_get_current_os_pkgtypes`
    exit_on_error $? "Failed to get current OS package types."
    for _result in ${_list} ; do
        break
    done
    echo "${_result}"
    return 0
}
