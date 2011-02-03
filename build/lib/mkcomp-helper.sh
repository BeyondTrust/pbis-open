#!/bin/bash

#
# Each component has a componentName.comp file and optionally
# a componentName.vars file.
#
# Each .comp file is a Bash scriptlet which is sourced to define
# the necessary functions.  These are the functions that must be defined:
#
# - component_gather
# - component_configure
# - component_build
# - component_stage
#
# Each .vars file is a Bash scriptlet that is sourced to define
# some optional variables:
#
# COMP_VERSION
# COMP_RELEASE
# COMP_NEED_COMPAT
#
#
# Variables that are defined for all (.comp and .vars):
#
# - vars from buildenv (BUILD_...)
# - COMP_NAME
#
# Variables defined for .comp calls:
#
# - GATHER_DIR (valid for gather only)
# - BUILDING_ROOT (valid for configure/build/stage)
# - STAGING_ROOT (valid for configure/build/stage)
# - PREFIX_DIR (valid for configure/build/stage)
# - IS_COMPAT (valid for configure/build/stage, as needed)
#
# - COMP_VERSION
# - COMP_RELEASE
#
# - STAGING_ROOT_DIR=${STAGING_ROOT}
# - STAGING_PREFIX_DIR=${STAGING_ROOT_DIR}/${PREFIX_DIR}
#

source "${BUILD_ROOT}/src/linux/build/lib/dep-helper.sh"

function _get_lib_dir
{
    local dir=
    case ${BUILD_OS_TYPE}-${BUILD_OS_ARCH}-$(if [ -n "${IS_COMPAT}" ]; then echo yes; else echo no; fi) in
        linux-x86_64-no)
            dir=lib64
            ;;
        linux-s390x-no)
            dir=lib64
            ;;
        hpux-*-yes)
            dir=lib64
            ;;
        solaris-*-yes)
            dir=lib64
            ;;
        aix-powerpc-yes)
            dir=lib64
            ;;
        *)
            dir=lib
            ;;
    esac
    echo "$dir"
}

function _get_base_compat_flags
{
    local flags=

    if [ -n "${COMP_IS_COMPAT}" ]; then
        case "${BUILD_OS_TYPE}-${BUILD_OS_ARCH}" in
	    darwin-x86_64)
		flags="${flags} -arch i386"
		;;
            linux-x86_64)        
                flags="${flags} -m32"
                ;;
            hpux-ia64)
                flags="${flags} -mlp64"
                ;;
            solaris-*)
                flags="${flags} -m64"
                ;;
            aix-powerpc)
                flags="${flags} -maix64"
                ;;
        esac
    fi
    echo "$flags"
}

function _get_base_cppflags
{
    local flags="${BUILD_CPPFLAGS}"

    echo "$flags"
}

function _get_base_cflags
{
    local flags="${BUILD_CFLAGS}"

    if [ -n "${BUILD_DEBUG}" ]; then
	flags="$flags -O0 -g -ggdb"
    elif [ -n "${BUILD_MINIMAL}" -o "${BUILD_OS_TYPE}" = solaris ]; then
	flags="$flags -Os -g"
    else
        flags="$flags -O2 -g"
    fi

    if [ -n "${BUILD_PROFILING}" ]; then
        flags="$flags -pg"
    fi

    case "${BUILD_OS_TYPE}" in
	linux)
	    if [ `uname -m` = i686 ]; then
		flags="$flags -m32"
	    fi
	    ;;
    esac

    flags="$flags -fmessage-length=0 -D_FORTIFY_SOURCE=2"

    echo "$flags"
}

function _get_base_ldflags
{
    local flags="${BUILD_LDFLAGS}"

    if [ -n "${BUILD_DEBUG}" ]
    then
        flags="$flags -O0 -ggdb"
    fi
    
    if [ -n "${BUILD_PROFILING}" ]
    then
        flags="$flags -pg"
    fi

    case "${BUILD_OS_TYPE}" in
        darwin)
            flags="$flags -Wl,-headerpad_max_install_names"
            ;;
	linux)
	    if [ -z "${OFFICIAL_BUILD_NUMBER}" ]
	    then
                flags="$flags -Wl,--enable-new-dtags"
	    fi
	    if [ `uname -m` = i686 ]; then
		flags="$flags -m32"
	    fi
	    ;;
    esac

    echo "$flags"
}

function _get_base_ldshflags
{
    local flags=
    if [ -n "${IS_COMPAT}" ]; then
        flags=$(_get_base_compat_flags)
    fi

    echo "$flags"
}

function _get_base_mflags
{
    local flags=
    if [ "${BUILD_OS_TYPE}" = 'linux' ]; then
        flags="-j$((`cat /proc/cpuinfo | grep '^processor' | wc -l` * 2))"
    elif [ "${BUILD_OS_TYPE}" = 'solaris' ]; then
        local num_procs=`/usr/sbin/psrinfo | /usr/bin/wc -l`
        if [ -n "${num_procs}" ]; then
            flags="-j$((${num_procs} * 2))"
        fi
    else
        flags="-j2"
    fi
    echo "$flags"
}

function _get_staging_root
{
    # a little bit of a hack

    #
    # STAGING_ROOT is something like .../<component>/<staging>, so
    # we need to ../.. the directory.  However, the directory may
    # not exist, so we need to remove the two trailing components
    # rather than using ../..
    #
    check_arg "component" "$1"

    if component_is_external "$1"
    then
	echo "`component_external_path $1`"
    else
	local temp=`dirname "${STAGING_ROOT}"`
	temp=`dirname "${temp}"`
	echo "${temp}/$1/`basename ${STAGING_ROOT}`"
    fi
}

function _get_staging_prefix_dir
{
    echo "`_get_staging_root $1`/${PREFIX_DIR}"
}

function _get_staging_root_dir
{
    echo "`_get_staging_root $1`"
}

function get_staging_prefix_dir
{
    ( set +x && _get_staging_prefix_dir "$@" )
}

function get_staging_root_dir
{
    ( set +x && _get_staging_root_dir "$@" )
}

function get_build_string
{
    ${BUILD_RUN_ROOT}/share/automake-*/config.guess
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
        local _dest_name
        for _path in ${_current}; do
            ( set -x && rsync -a "${_path}" "${_last}" )
            exit_on_error $?
            if [ -d "${_last}" ]; then
                _dest_name="${_last}/`basename ${_path}`"
            else
                _dest_name="${_last}"
            fi
            # Only change the permissions if the file is not a symlink
            if [ ! -L "$_dest_name" ]; then
                ( set -x && chmod "${_mode}" "${_dest_name}" )
                exit_on_error $?
            fi
        done
    done
}

function x_cp
{
    ( set +x && _x_cp "$@" )
}

# function x_strip
# {
#     check_arg "path" "$1"
#     strip "$@"
#     exit_on_error $?
# }

function x_strip_all_exec
{
    check_arg "path" "$1"
    find "$1" -perm +x -type f | xargs strip
    exit_on_error $?
}

function _invoke
{
    local _function="component_${_COMP_OP}"
    exists_function "${_function}"
    if [ $? -eq 0 ]; then
        ( set -ex && ${_function} "$@" )
        exit_on_error $?
    else
        warn "${_function} does not exist"
        exit 1
    fi
}

function _set_build_roots
{
    # Verify the prefix directory used to configure, etc.
    check_arg PREFIX_DIR "${PREFIX_DIR}"

    check_dir_arg BUILDING_ROOT "${BUILDING_ROOT}"
}

function _set_dependency_tracking
{
    DEPENDENCY_TRACKING=""

    case "${BUILD_OS_TYPE}-${BUILD_OS_ARCH}" in
        darwin-x86_64)
            DEPENDENCY_TRACKING="--disable-dependency-tracking"
            ;;
    esac
}

function _set_stage_roots
{
    _set_build_roots

    check_dir_arg STAGING_ROOT "${STAGING_ROOT}"

    #STAGING_PREFIX_DIR=${STAGING_ROOT}/${PREFIX_DIR}
    #STAGING_ROOT_DIR=${STAGING_ROOT}

    STAGING_PREFIX_DIR=${STAGING_ROOT}/${PREFIX_DIR}
    STAGING_ROOT_DIR=${STAGING_ROOT}
}


function _invoke_op_internal
{
    check_arg BUILD_ROOT "${BUILD_ROOT}"
    check_arg BUILD_OS_TYPE "${BUILD_OS_TYPE}"
    check_arg BUILD_OS_ARCH "${BUILD_OS_ARCH}"

    check_arg _COMP_OP "${_COMP_OP}"
    check_arg _COMP_FILE "${_COMP_FILE}"
    check_arg COMP_NAME "${COMP_NAME}"

    if [ "${BUILD_OS_TYPE}" = "aix" ]; then
        export PATH="${BUILD_ROOT}/src/linux/build/lib/gcc-wrap:${PATH}"
    fi

    _lib=`_get_lib_dir`
    _cppflags=`_get_base_cppflags`
    _cflags=`_get_base_cflags`
    _mflags=`_get_base_mflags`
    _ldshflags=`_get_base_ldshflags`
    _ldflags=`_get_base_ldflags`

    source "${_COMP_FILE}"
    exit_on_error $? "Missing ${_COMP_FILE}"

    case "${_COMP_OP}" in
        exists)
            exists_function "component_${1}"
            exit $?
            ;;

	bootstrap)
	    _invoke "$@"
	    ;;

        gather)
            check_arg GATHER_DIR "$GATHER_DIR"
            _invoke "$@"
            ;;

	configure)
            _set_dependency_tracking
            _set_build_roots
            _invoke "$@"
            ;;

        build)
            _set_build_roots
            _invoke "$@"
            ;;

        stage)
            _set_stage_roots
            _invoke "$@"
            ;;

        rpm_to_deb_filter)
            check_arg CONTROL_FILE "$CONTROL_FILE"
            _invoke "${CONTROL_FILE}" "$@"
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

    check_arg _COMP_FILE "${_COMP_FILE}"
    check_arg COMP_NAME "${COMP_NAME}"

    COMP_VERSION=
    COMP_RELEASE=
    COMP_NEED_COMPAT=
    COMP_SOURCES=
    COMP_IS_TOOL=
    COMP_INCREMENTAL=
    COMP_DISABLE_CACHE=

    source "${_COMP_FILE}"
    exit_on_error $? "Missing ${_COMP_FILE}"

    echo "COMP_VERSION=${COMP_VERSION}"
    echo "COMP_RELEASE=${COMP_RELEASE}"
    echo "COMP_NEED_COMPAT=${COMP_NEED_COMPAT}"
    echo "COMP_NEED_FOREIGN=${COMP_NEED_FOREIGN}"
    echo "COMP_SOURCES=${COMP_SOURCES}"
    echo "COMP_IS_TOOL=${COMP_IS_TOOL}"
    echo "COMP_INCREMENTAL=${COMP_INCREMENTAL}"
    echo "COMP_DISABLE_CACHE=${COMP_DISABLE_CACHE}"

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

_LIBTOOL_REWRITE=${BUILD_ROOT}/src/linux/build/lib/libtool-dependency-rewrite.sh
_DYLIB_REWRITE=${BUILD_ROOT}/src/linux/build/lib/dylib-dependency-rewrite.sh
_LIBTOOL_DEPS=${BUILD_ROOT}/src/linux/build/lib/libtool-dependency-dirs.sh

function _libtool_staging_libdirs
{
    local dep
    local depdir
    (
	local closure="`component_transitive_closure ${COMP_NAME}`"
        for dep in ${closure}
        do
	    if ! component_is_external "${dep}"
	    then
		if component_is_tool "${dep}"
		then
                    local depdir="${BUILD_RUN_ROOT}/${_lib}"
		    if [ "${dep}" = "${COMP_NAME}" ]
		    then
			depdir="${depdir} $(get_staging_root_dir $dep)/${BUILD_RUN_ROOT}/${_lib}"
		    fi
		else
                    local depdir="$(get_staging_prefix_dir $dep)/${_lib}"
		fi
		echo $depdir
	    fi
        done
    ) | sed 's://*:/:g' | sort | uniq
    (
        for dep in ${closure}
        do
	    if component_is_external "${dep}"
	    then
		echo "`component_external_path ${dep}`/${_lib}"
	    fi
        done
    ) | sed 's://*:/:g' | sort | uniq
}

function _libtool_staging_dirs
{
    echo ${STAGING_ROOT_DIR}
    _libtool_staging_libdirs | sed "s:${PREFIX_DIR}/${_lib}::g"
}

function libtool_staging_dirs
{
    ( set +x && _libtool_staging_dirs "$@" )
    exit_on_error $?
    return 0
}

function _libtool_staging_ldflags
{
    flags=""
 
    for dir in $(_libtool_staging_libdirs)
    do
        flags="$flags -L$dir"
        case "${BUILD_OS_TYPE}" in
            linux|freebsd)
                flags="$flags -Wl,-rpath-link -Wl,$dir"
                ;;
        esac
    done

    flags="$flags -L${STAGING_ROOT}/${PREFIX_DIR}/${_lib}"

    if test "${BUILD_OS_TYPE}" = "aix"
    then
        flags="$flags -Wl,-brtl"
    fi

    echo $flags
}

function libtool_staging_ldflags
{
    ( set +x && _libtool_staging_ldflags "$@" )
    exit_on_error $?
    return 0
}

function _libtool_staging_cppflags
{
    flags=""
 
    for dir in $(_libtool_staging_libdirs)
    do
        flags="$flags -I$(echo $dir | sed s:/${_lib}$:/include:)"
    done

    echo $flags
}

function libtool_staging_cppflags
{
    ( set +x && _libtool_staging_cppflags "$@" )
    exit_on_error $?
    return 0
}

function _libtool_staging_library_path
{
    local dirs=`_libtool_staging_libdirs`
    echo $dirs | sed 's/  */:/g'
}

function libtool_staging_library_path
{
    ( set +x && _libtool_staging_library_path "$@" )
    exit_on_error $?
    return 0
}

function _libtool_staging_path
{
    local dir;

    (
	for dir in `_libtool_staging_dirs`
	do
	    if [ -d "$dir/${PREFIX_DIR}/bin" ]
	    then
		echo "$dir/${PREFIX_DIR}/bin"
	    fi
        done
    ) | xargs | sed 's/ /:/g'    
}

function libtool_staging_path
{
    ( set +x && _libtool_staging_path "$@" )
    exit_on_error $?
    return 0
}

function libtool_set_staging_library_path
{
    set +x
    local libpath=`libtool_staging_library_path`
    
    case "${BUILD_OS_TYPE}" in
        darwin)
            export DYLD_LIBRARY_PATH="${libpath}"
            ;;
	hpux)
	    export SHLIB_PATH="${libpath}"
	    export LD_LIBRARY_PATH="${libpath}"
	    ;;
        *)
            export LD_LIBRARY_PATH="${libpath}"
            ;;
    esac
    set -x
}

function libtool_set_staging_path
{
    set +x
    local path=`libtool_staging_path`
    
    export PATH=${path}:${PATH}
    set -x
}

function _libtool_rewrite_staging
{
    local staging_list=$(libtool_staging_dirs)

    if echo ${STAGING_PREFIX_DIR}/${_lib}/*.la | grep -v '*' >/dev/null 2>&1
    then
        for la in ${STAGING_PREFIX_DIR}/${_lib}/*.la
          do
          $_LIBTOOL_REWRITE -staging ${la} ${PREFIX_DIR} $(echo $staging_list | sed 's/  */:/g')
        done
    fi


    if test "${BUILD_OS_TYPE}" = "darwin"
    then
        for binary in ${STAGING_PREFIX_DIR}/${_lib}/{/,grouppolicy}/*.{so,dylib}* ${STAGING_PREFIX_DIR}/{sbin,bin}/* ${STAGING_ROOT_DIR}/${_lib}/{/,security}/*.{so,dylib}*
        do
          if [ -f "$binary" ] && file "$binary" | grep "Mach-O" >/dev/null 2>&1
          then
              $_DYLIB_REWRITE -staging ${binary} ${PREFIX_DIR}/${_lib} $(echo $staging_list | sed 's/  */:/g')
          fi
        done
    fi
}

function libtool_rewrite_staging
{
    ( set +x && _libtool_rewrite_staging "$@" )
    exit_on_error $?
    return 0
}

function _libtool_rewrite_populate
{
    local staging_list="$(_libtool_staging_dirs)"
    local rewrite_paths="$(echo ${staging_list} ${BUILD_RUN_ROOT} | sed 's/  */:/g')"
    
    if [ "${PKG_TYPE}" = "platform" ]
    then
	DEST_DIR="`cd "${PKG_DIR}" && pwd`"
    else
	DEST_DIR=""
    fi

    local la
    for la in ${POPULATE_PREFIX_DIR}/${_lib}/*.la
    do
	if [ -f "$la" ]
	then
	    $_LIBTOOL_REWRITE -final ${la} ${PREFIX_DIR} ${rewrite_paths} ${DEST_DIR}
	fi
    done
    
    if test "${BUILD_OS_TYPE}" = "darwin"
    then
	local rewrite_paths="$(echo ${staging_list} | sed 's/  */:/g')"
        for binary in ${POPULATE_PREFIX_DIR}/${_lib}/{/,grouppolicy}/*.{so,dylib}* ${POPULATE_PREFIX_DIR}/{sbin,bin}/* ${POPULATE_ROOT_DIR}/${_lib}/*.{so,dylib}* ${POPULATE_ROOT_DIR}/${_lib}/security/*.{so,dylib}*
        do
	    if [ -x "$binary" ] && file "$binary" | grep "Mach-O" >/dev/null 2>&1
	    then
		$_DYLIB_REWRITE -final ${binary} ${PREFIX_DIR}/${_lib} ${rewrite_paths} ${DEST_DIR}
	    fi
        done
    fi
}

function libtool_rewrite_populate
{
    ( set +x && _libtool_rewrite_populate "$@" )
    exit_on_error $?
    return 0
}

function set_compiler_env
{
    GCC="gcc"
    GPP="g++"

    # On HP-UX PA-RISC, we have to use a special
    # version of gcc to do 64-bit compat
    if [ "${BUILD_OS_TYPE}" = "hpux" ] &&
       [ "${BUILD_OS_ARCH}" = "hppa20" ] &&
       [ -n "${IS_COMPAT}" ]; then
        GCC="/opt/hp-gcc64/bin/gcc"
    fi
    
    CC="${GCC} -pipe $(_get_base_compat_flags)"
    CXX="${GPP} -pipe $(_get_base_compat_flags)"
    MAKE=gmake
    
    case "${BUILD_OS_TYPE}" in
        linux|darwin) 
            MAKE=make
            ;;
        *)
            CC="$CC -static-libgcc"
            ;;
    esac
    
    # Collapse spaces to stop krb5 configure
    # script from complaining about CC changing
    CC=`echo $CC | sed 's/  */ /g'`

    # Support for compile wrappers such as ccache
    if [ -n "${BUILD_COMPILE_PREFIX}" ]; then
        CC="${BUILD_COMPILE_PREFIX} ${CC}"
        CXX="${BUILD_COMPILE_PREFIX} ${CXX}"
    fi

    export CC CXX MAKE

    # On Aix, we have to tell 'ar' et al to deall with 64-bit when doing COMPAT
    if [ -n "${COMP_IS_COMPAT}" ]; then
        case "${BUILD_OS_TYPE}-${BUILD_OS_ARCH}" in
            aix-powerpc)
                OBJECT_MODE="64"
                export OBJECT_MODE
                ;;
        esac
    fi

    echo "$flags"

    return 0
}

function _libtool_format_rpath
{
    local dir=$1
    local result

    case "${BUILD_OS_TYPE}" in
        linux)
            result="-Wl,-rpath -Wl,$dir"
            ;;
        solaris)
            result="-R$dir"
            ;;
        freebsd)
            result="-Wl,-rpath -Wl,$dir"
            ;;
        aix)
            result="-Wl,-R,$dir -Wl,-brtl"
            ;;
        hpux)
            result="-Wl,+b -Wl,$dir"
            ;;
        darwin)
            result=""
            ;;
    esac
    
    echo "$result"
}

function libtool_format_rpath
{
    ( set +x && _libtool_format_rpath "$@" )
    exit_on_error $?
    return 0
}

function _libtool_format_ldpath
{
    local dir=$1
    local result

    case "${BUILD_OS_TYPE}" in
        linux|freebsd)
            result="-L$dir -Wl,-rpath-link -Wl,$dir"
            ;;
        *)
            result="-L$dir"
            ;;
    esac
    
    echo "$result"
}


function libtool_format_ldpath
{
    ( set +x && _libtool_format_ldpath "$@" )
    exit_on_error $?
    return 0
}
