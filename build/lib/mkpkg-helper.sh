#!/bin/bash

#
# Each package has a packageName.vars file.
#
# Each .vars file is a Bash scriptlet that is sourced to define
# some optional variables:
#
# PKG_NAME
# PKG_VERSION
# PKG_RELEASE
# PKG_NEED_COMPAT
#
# Variables that are defined for scriptlet:
#
# - vars from buildenv (BUILD_...)
# - PKG_ALIAS
# - PKG_TYPE
#
# ....
#

source "${BUILD_ROOT}/src/linux/build/lib/dep-helper.sh"

function _get_lib_dir
{
    local dir=
    _is_compat=$(if [ -n "${IS_COMPAT}" ]; then echo yes; else echo no; fi)
    _is_foreign=$(if [ -n "${IS_FOREIGN}" ]; then echo yes; else echo no; fi)
    case ${BUILD_OS_TYPE}-${BUILD_OS_ARCH}-${_is_compat}-${_is_foreign} in
        linux-x86_64-no-*)
            dir=lib64
            ;;
        linux-s390x-no-*)
            dir=lib64
            ;;
        hpux-ia64-yes-*)
            dir=lib64
            ;;
        hpux-hppa20-yes-*)
            dir=lib64
            ;;
        solaris-sparc-yes-*)
            dir=lib64
            ;;
        solaris-i386-*-yes|solaris-i386-yes-*)
            dir=lib64
            ;;
        *)
            dir=lib
            ;;
    esac
    echo "$dir"
}

function _get_nss_dir
{
    local _lib=`_get_lib_dir`
    if [ -z "${_lib}" ]; then
        exit_on_error 1 "Missing lib dir"
    fi
    local dir=/${_lib}
    case "${BUILD_OS_TYPE}-${BUILD_OS_ARCH}" in
        aix-*)
            dir=/usr/${_lib}/security
            ;;
        solaris-i386)
            if [ -n "${IS_FOREIGN}" ]
            then
                dir="/usr/lib/amd64"
            else
                dir="/usr/lib"
            fi
            ;;
        solaris-sparc)
            if [ -n "${IS_COMPAT}" ]
            then
                dir="/usr/lib/sparcv9"
            else
                dir="/usr/lib"
            fi
            ;;
        hpux-ia64)
            if [ -n "${IS_FOREIGN}" ]
            then
                dir="/usr/lib /usr/lib/pa20_32"
            elif [ -z "${IS_COMPAT}" ]
            then
                dir=/usr/lib/hpux32
            else
                dir=/usr/lib/hpux64
            fi
            ;;
        hpux-hppa20)
            if [ -n "${IS_COMPAT}" ]
            then
                dir="/usr/lib/pa20_64"
            else
                dir="/usr/lib /usr/lib/pa20_32"
            fi
            ;;
        darwin-*)
            dir="/usr/lib"
            ;;
        freebsd-*)
            if [ -n "${IS_COMPAT}" ]
            then
                dir="FIXME"
            else
                dir="/usr/local/lib"
            fi
    esac
    echo "$dir"
}

function _get_pam_dir
{
    local _lib=`_get_lib_dir`
    if [ -z "${_lib}" ]; then
        exit_on_error 1 "Missing lib dir"
    fi
    local dir=/${_lib}/security
    case "${BUILD_OS_TYPE}-${BUILD_OS_ARCH}" in
        aix-*)
            dir=/usr/${_lib}/security
            ;;
        solaris-i386)
            if [ -n "${IS_FOREIGN}" ]
            then
                dir="/usr/lib/security/amd64"
            else
                dir="/usr/lib/security"
            fi
            ;;
        solaris-sparc)
            if [ -n "${IS_COMPAT}" ]
            then
                dir="/usr/lib/security/sparcv9"
            else
                dir="/usr/lib/security"
            fi
            ;;
        hpux-ia64)
            if [ -n "${IS_FOREIGN}" ]
            then
                dir="/usr/lib/security"
            elif [ -z "${IS_COMPAT}" ]
            then
                dir=/usr/lib/security/hpux32
            else
                dir=/usr/lib/security/hpux64
            fi
            ;;
        hpux-hppa20)
            if [ -n "${IS_COMPAT}" ]
            then
                dir="/usr/lib/security/pa20_64"
            else
                dir="/usr/lib/security"
            fi
            ;;
        darwin-*)
            dir="/usr/lib/pam"
            ;;
        freebsd-*)
            if [ -n "${IS_COMPAT}" ]
            then
                dir="FIXME"
            else
                dir="/usr/local/lib"
            fi
            ;;
    esac
    echo "$dir"
}

function _get_init_dir
{
    local dir=/etc/init.d
    # On some Linux systems (FC5), RPM defines _initrddir
    # %{_sysconfdir}/rc.d/init.d.  However, we use /etc/init.d
    # on all Linux systems because they always have an /etc/init.d
    # which may be a symlink on some systems.  Note that we cannot
    # use /etc/rc.d/init.d on all Linux systems because some systems
    # (such as Ubuntu) do not have an /etc/rc.d directory.
    case "${BUILD_OS_TYPE}" in
	freebsd)
	    dir=/etc/rc.d
	    ;;
        aix)
            dir=/etc/rc.d/init.d
            ;;
	hpux)
	    dir=/sbin/init.d
	    ;;
    esac
    echo "$dir"
}

function _get_comp_staging_prefix_dir
{
    check_arg "component" "$1"
    echo "${COMP_STAGING_PREFIX}/$1/${COMP_STAGING_SUFFIX}/${PREFIX_DIR}"
}

function _get_comp_staging_root_dir
{
    check_arg "component" "$1"
    echo "${COMP_STAGING_PREFIX}/$1/${COMP_STAGING_SUFFIX}"
}

function get_comp_staging_prefix_dir
{
    ( set +x && _get_comp_staging_prefix_dir "$@" )
}

function get_comp_staging_root_dir
{
    ( set +x && _get_comp_staging_root_dir "$@" )
}

function _x_cp
{
    check_arg "mode" "$1"
    check_arg "source" "$2"
    check_arg "target" "$3"

    local _mode="$1"
    shift

    local _last=""
    local -i _index
    local _current

    _index=0
    for _current in "$@"; do
        let _index=${_index}+1
        if [ ${_index} = "$#" ]; then
            _last="${_current}"
            break;
        fi
    done

    _index=0
    for _current in "$@"; do
        _index=${_index}+1
        if [ ${_index} = "$#" ]; then
            break;
        fi
        local _path
        for _path in ${_current}; do
            if [ -d "${_last}" ]; then
                ( set -x && cp -p "${_path}" "${_last}" )
                exit_on_error $?
                ( set -x && chmod "${_mode}" "${_last}/`basename ${_path}`" )
                exit_on_error $?
            else
                ( set -x && cp -p "${_path}" "${_last}" )
                exit_on_error $?
                ( set -x && chmod "${_mode}" "${_last}" )
                exit_on_error $?
            fi
        done
    done
}

function x_cp
{
    ( set +x && _x_cp "$@" )
}

function _x_rm
{
    local file
    for file in "$@"
    do
        if [ -e "$file" ]
        then
            echo "x_rm: $file"
            rm -f "$file"
        fi
    done
}

function x_rm
{
    ( set +x && _x_rm "$@" )
}

function _x_rmdir
{
    local dir
    for dir in "$@"
    do
        if [ -d "$dir" ]
        then
            echo "x_rmdir: $dir"
            rm -rf "$dir"
        fi
    done
}

function x_rmdir
{
    ( set +x && _x_rmdir "$@" )
}

function _x_mkinit
{
    local from="$1"
    local script="$2"
    local sPriAix="$3"
    local kPriAix="$4"
    local scriptSolaris="$5"
    local sPriSolaris="$6"
    local kPriSolaris="$7"

    check_arg from "${from}"
    check_arg script "${script}"
    check_arg sPriAix "${sPriAix}"
    check_arg kPriAix "${kPriAix}"
    check_arg scriptSolaris "${scriptSolaris}"
    check_arg sPriSolaris "${sPriSolaris}"
    check_arg kPriSolaris "${kPriSolaris}"

    ( set -x && mkdir -p ${POPULATE_ROOT_DIR}/${INIT_DIR} )
    ( set -x && sed "s#PREFIX_DIR#${PREFIX_DIR}#g" ${from} > ${POPULATE_ROOT_DIR}/${INIT_DIR}/${script} )
    ( set -x && chmod 0755 ${POPULATE_ROOT_DIR}/${INIT_DIR}/${script} )
}

function x_mkinit
{
    if [ "${BUILD_OS_TYPE}" != 'darwin' ] ; then
       ( set +x && _x_mkinit "$@" )
    fi
}


function _matches
{
    local search="$1"
    shift

    for pattern in "$@"
    do
        if echo "$search" | grep "$pattern" >/dev/null 2>&1
        then
            return 0
        fi
    done
    
    return 1
}

function _is_strippable
{
    local file="$1"
    if ! test -f "$file"; then return 1; fi
    if ! test -x "$file"; then return 1; fi
    if test -h "$file"; then return 1; fi
    local info=`file "$file"`
    if _matches "$info" "not stripped" "ELF" "object file" "shared library" "shared executable"
    then
        if _matches "$info" "stripped" && ! _matches "$info" "not stripped"
        then
            return 1
        else
            return 0
        fi
    else
        return 1
    fi
}

function _x_strip
{
    local STRIP
    local OBJCOPY=""

    case "${BUILD_OS_TYPE}" in
	darwin)
	    STRIP="strip -S -x"
	    ;;
	*)
	    STRIP="strip"
	    ;;
    esac

    case "${BUILD_OS_TYPE}" in
	linux)
	    OBJCOPY="objcopy"
	    ;;
    esac

    if ! [ -n "${BUILD_DEBUG}" ]
    then
        local file
        for file in "$@"
        do
            if _is_strippable "$file"
            then
                local writeable
                local debugname
                local debugdir
                echo x_strip: $file
                if ! test -w "$file"
                then
                    writeable=false
                    chmod u+w "$file"
                else
                    writeable=true
                fi
                if [ -n "${OBJCOPY}" -a -n "${POPULATE_DEBUG_DIR}" ] ; then
                    debugdir=`dirname "$file"`
                    debugdir=`echo ${debugdir} | sed -e "s:${POPULATE_ROOT_DIR}::"`
                    debugdir="${POPULATE_DEBUG_DIR}/${debugdir}"
                    mkdir -p "${debugdir}"
                    debugname=`basename "$file"`.dbg
                    ${OBJCOPY} --only-keep-debug "$file" "${debugdir}/${debugname}"
                fi
                ${STRIP} "$file"
                if [ -n "${OBJCOPY}" -a -n "${POPULATE_DEBUG_DIR}" ] ; then
                    (cd "${debugdir}" && ${OBJCOPY} --add-gnu-debuglink="${debugname}" "$file" )
                fi
                if ! ${writeable}
                then
                    chmod u-w "$file"
                fi
            elif test -d "$file"
            then
                _x_strip $file/*
            fi
        done
    fi
}

function x_strip
{
    ( set +x && _x_strip "$@" )
}

function _strip_common
{
    local dir

    x_strip ${POPULATE_ROOT_DIR}/{/,usr,$PREFIX_DIR}/{bin,sbin,$_lib}
    [ "${_lib}" = "lib" ] || x_strip ${POPULATE_ROOT_DIR}/usr/lib

    local dirs="${POPULATE_PREFIX_DIR}/${_lib}"

    for dir in ${NSS_DIR} ${PAM_DIR}
    do
        dirs="$dirs ${POPULATE_ROOT_DIR}/${dir}"
    done

    for dir in ${dirs}
    do
	x_rm ${dir}/.libs
	x_rm ${dir}/*_mu.*
    done
}

function strip_common
{
    ( set +x && _strip_common )
}

function _populate_component
{
    local _name="$1"

    # Skip populating component if it is externally satisfied
    if component_is_external "${_name}"
    then
        echo "Skipping populating component $1 since it is externally satisfied"
	return 0
    fi

    local _path="${BUILD_ROOT}/src/linux/build/components/${_name}.comp"
    local -x comp_needs_compat=`(source "${_path}" && echo "$COMP_NEED_COMPAT")`
    local -x comp_needs_foreign=`(source "${_path}" && echo "$COMP_NEED_FOREIGN")`
    local -x comp_staging_path=`_get_comp_staging_root_dir ${_name}`

    if [ -n "${IS_COMPAT}" -a -z "${comp_needs_compat}" ]; then
        echo "Skipping populating component $1 since it doesn't need compat"
        return 0
    fi

    if [ -n "${IS_FOREIGN}" -a -z "${comp_needs_foreign}" ]; then
        echo "Skipping populating component $1 since it doesn't need foreign"
        return 0
    fi

    if [ -n "${IS_FOREIGN}" ]; then
        mkdir -p ${comp_staging_path}
        if [ ! -d ${comp_staging_path} ]; then
            echo "Could not find ${comp_staging_path}"
            echo "WARNING: Skipping populating foreign component ${_name} as no staging directory was found"
            return 0
        fi
    fi

    if [ -f "${_path}" ]; then
        ( PKG_POPULATE_FRAGMENT=1 &&
          STAGING_ROOT=$(_get_comp_staging_root_dir $_name) &&
          STAGING_ROOT_DIR=$(_get_comp_staging_root_dir $_name) &&
          STAGING_PREFIX_DIR=$(_get_comp_staging_prefix_dir $_name) &&
          source ${BUILD_ROOT}/src/linux/build/lib/mkcomp-helper.sh &&
          source "${_path}" && set -x && component_populate "$@" )
        exit_on_error $?
    else
        echo "Missing file: ${_path}"
        echo "Current directory is: `pwd`"
        exit 1
    fi
}

function populate_component
{
    ( set +x && _populate_component "$@" )
}

function _populate_components
{
    local comp

    for comp in ${PKG_COMPONENTS}
    do
	_populate_component "${comp}"
    done
}

function populate_components
{
    ( set +x && _populate_components "$@" )
}

function _set_stage_roots
{
    _set_build_roots

    check_dir_arg STAGING_ROOT "${STAGING_ROOT}"

    STAGING_PREFIX_DIR=${STAGING_ROOT}/${PREFIX_DIR}
    STAGING_ROOT_DIR=${STAGING_ROOT}
}

function _invoke
{
    local _function="package_${_OP}"
    exists_function "${_function}"
    if [ $? -eq 0 ]; then
        ( set -ex && ${_function} "$@" )
        exit_on_error $?
    else
        warn "${_function} does not exist"
        exit 1
    fi
}

function _invoke_op_internal
{
    check_arg BUILD_ROOT "${BUILD_ROOT}"
    check_arg BUILD_OS_TYPE "${BUILD_OS_TYPE}"
    check_arg BUILD_OS_ARCH "${BUILD_OS_ARCH}"

    check_arg _OP "${_OP}"
    check_arg _FILE "${_FILE}"
    check_arg _DIR "${_DIR}"
    check_arg PKG_ALIAS "${PKG_ALIAS}"
    check_arg PREFIX_DIR "${PREFIX_DIR}"
    check_arg POPULATE_ROOT "${POPULATE_ROOT}"
    check_arg COMP_STAGING_PREFIX "${COMP_STAGING_PREFIX}"
    check_arg COMP_STAGING_SUFFIX "${COMP_STAGING_SUFFIX}"

    if [ "${BUILD_OS_TYPE}" = "aix" ]; then
        export PATH="${BUILD_ROOT}/src/linux/build/lib/lppbuild:${PATH}"
    fi

    _lib="`_get_lib_dir`"
    NSS_DIR="`_get_nss_dir`"
    PAM_DIR="`_get_pam_dir`"
    INIT_DIR="`_get_init_dir`"

    PKG_POPULATE_FRAGMENT=""

    source "${_FILE}"
    exit_on_error $? "Missing ${_FILE}"

    case "${_OP}" in
        exists)
            exists_function "package_${1}"
            exit $?
            ;;

        rpm2deb_filter)
            check_arg CONTROL_FILE "$1"
            _invoke "$@"
            ;;

        pkg_populate)
            check_arg PROTO_DIR "$1"
            _invoke "$@"
            ;;

        populate)
            SCRIPT_DIR="${_DIR}"
            check_arg POPULATE_PREFIX_DIR "${POPULATE_PREFIX_DIR}"
            check_arg POPULATE_ROOT_DIR "${POPULATE_ROOT_DIR}"
            _invoke "$@"
            ;;

        *)
            warn "Invalid op: $_COMP_OP"
            exit 1
            ;;
    esac

    return 0
}


function _invoke_vars_internal
{
    check_arg BUILD_ROOT "${BUILD_ROOT}"
    check_arg BUILD_OS_TYPE "${BUILD_OS_TYPE}"
    check_arg BUILD_OS_ARCH "${BUILD_OS_ARCH}"

    check_arg _FILE "${_FILE}"
    check_arg PKG_ALIAS "${PKG_ALIAS}"

    PKG_VERSION=
    PKG_RELEASE=
    PKG_NEED_COMPAT=
    PKG_NEED_FOREIGN=
    PKG_IS_META=

    source "${_FILE}"
    exit_on_error $? "Missing ${_FILE}"

    echo "PKG_VERSION=${PKG_VERSION}"
    echo "PKG_RELEASE=${PKG_RELEASE}"
    echo "PKG_DESC=${PKG_DESC}"
    echo "PKG_NEED_COMPAT=${PKG_NEED_COMPAT}"
    echo "PKG_NEED_FOREIGN=${PKG_NEED_FOREIGN}"
    echo "PKG_IS_META=${PKG_IS_META}"

    return 0
}


function _invoke_op
{
    ( set +e && _invoke_op_internal "$@" )
    exit_on_error $?
    return 0
}


function _invoke_vars
{
    ( set +e && _invoke_vars_internal "$@" )
    exit_on_error $?
    return 0
}
