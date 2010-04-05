#!/bin/bash

source "${BUILD_ROOT}/src/linux/build/lib/vc-helper.sh"

PRODUCTS_DIR=${BUILD_ROOT}/src/linux/build/products
PACKAGES_DIR=${BUILD_ROOT}/src/linux/build/packages
COMPONENTS_DIR=${BUILD_ROOT}/src/linux/build/components

_capitalize()
{
    local first="`echo ${1:0:1} | tr '[:lower:]' '[:upper:]'`"

    echo "${first}${1:1}"
}

_unique_list()
{
    xargs | awk 'BEGIN { RS=" "; } { print $1; }' | sort | uniq | xargs
}

_contains()
{
    local needle=$1
    local hay
    shift

    for hay in $@
    do
	test "$needle" = "$hay" && return 0
    done

    return 1
}

product_list()
{
    for dir in ${PRODUCTS_DIR}/*
    do
	if [ -f "$dir/config" ]
	then
	    basename $dir
	fi
    done
}

product_name()
{
    echo "`( source ${PRODUCTS_DIR}/$1/config && echo $NAME )`"
}

product_full_name()
{
    echo "`( source ${PRODUCTS_DIR}/$1/config && echo $FULL_NAME )`"
}

product_description()
{
    echo "`( source ${PRODUCTS_DIR}/$1/config && echo $DESCRIPTION )`"
}

product_files()
{
    echo "`( source ${PRODUCTS_DIR}/$1/config && echo $FILES )`"
}

product_script()
{
    echo "`( source ${PRODUCTS_DIR}/$1/config && echo $SCRIPT )`"
}

product_daemons()
{
    echo "`( source ${PRODUCTS_DIR}/$1/config && echo $DAEMONS )`"
}

product_obsolete_daemons()
{
    echo "`( source ${PRODUCTS_DIR}/$1/config && echo $OBSOLETE_DAEMONS )`"
}

product_autostart()
{
    echo "`( source ${PRODUCTS_DIR}/$1/config && echo $AUTOSTART )`"
}

product_regfiles()
{
    echo "`( source ${PRODUCTS_DIR}/$1/config && echo $REGFILES )`"
}

product_packages()
{
    local pkgs;

    (
	for prod in $@
	do
	    ( source ${PRODUCTS_DIR}/${prod}/config && echo $PACKAGES )
	done
    ) | _unique_list 
}

product_native_packages()
{
    (
	for pkg in `( source ${PRODUCTS_DIR}/${1}/config && echo $PACKAGES )`
	do
	    package_native_name "${pkg}"
	done
    ) | xargs
}

product_native_obsolete_packages()
{
    (
	for pkg in `( source ${PRODUCTS_DIR}/${1}/config && echo $OBSOLETE_PACKAGES )`
	do
	    package_native_name "${pkg}"
	done
    ) | xargs
}

product_native_packages_compat()
{
    if [ "${PKG_TYPE}" = "rpm" -a "${BUILD_OS_TYPE}" = "linux" ]
    then
    (
	local pkg
	local is_compat
	for pkg in `( source ${PRODUCTS_DIR}/${1}/config && echo $PACKAGES )`
	do
	    is_compat=`( source ${PACKAGES_DIR}/${pkg}/${pkg}.func && echo $PKG_NEED_COMPAT )`
	    if [ -n "$is_compat" ]
	    then
		echo "`package_native_name "${pkg}"`-32bit"
	    fi
	done
    ) | xargs
    fi
}

product_packages_spec()
{
    local _os="$1"
    local _arch="$2"
    local _pkg_type="$3"
    local _name="$4"

    local _list=`product_packages "${_name}"`

    local skip=""
    local spec=""
    local pkg
    for pkg in ${_list}; do
        local dir="${BUILD_DIST_ROOT}/${_os}/${_arch}/packages/${pkg}/${_pkg_type}"
        if [ ! -d "${dir}" ]; then
            skip=1
            break
        else
            spec="${spec} ${dir}/*.*"
        fi
    done

    if [ -z "${skip}" ]; then
        echo "${spec}"
    fi
    return 0
}

package_native_name()
{
    local name="$1"
    local vendorname="likewise"

    case ${PKG_TYPE} in
        rpm|deb|mac|freebsd|install|source)
	    echo "${vendorname}-${name}"
	    ;;
	bff)
	    echo "${vendorname}.`echo ${name} | sed 's/-/./g'`"
	    ;;
	pkg|depot)
	    local parts=`echo ${name} | sed 's/-/ /g'`
	    local concat=""
	    local part

	    for part in ${parts}
	    do
		concat="$concat`_capitalize $part`"
	    done
	    echo "`_capitalize "${vendorname}"`${concat}"
	    ;;
    esac
}

package_components()
{
    echo "`( source ${PACKAGES_DIR}/$1/$1.func && echo $PKG_COMPONENTS )`"
}

product_components()
{
    local pkg;

    (
	for pkg in `product_packages $@`
	do
	    package_components $pkg
	done
    ) | _unique_list
}

component_available()
{
    local comp_file

    for comp_file in ${BUILD_ROOT}/src/linux/build/components/*.comp
    do
	basename "${comp_file}" | sed 's/\.comp$//'
    done
}

component_dependencies()
{
    ( COMP_DEPENDENCIES=""; source ${COMPONENTS_DIR}/$1.comp && echo $COMP_DEPENDENCIES )
}

component_transitive_closure()
{
    local remaining="$@"

    (
	while [ -n "${remaining}" ]
	do
	    local process="${remaining}"
	    local comp	    

	    remaining=""
	    
	    for comp in ${process}
	    do
		local comp_normal="`echo ${comp} | sed 's/[-]/_/g'`"
		if [ -z "`eval 'echo $seen_'"${comp_normal}"`" ]
		then
		    echo "$comp"
		    eval "local seen_${comp_normal}=yes"
		    remaining="$remaining `component_dependencies "$comp"`"
		fi
	    done
	done
    ) | xargs
}

component_build_list()
{
    local comp
    local dep
    local sub
    local res=""

    for comp in $@
    do
	for dep in `component_dependencies $comp`
	do
	    if ! _contains $dep $res
	    then
		for sub in `component_build_list $dep`
		do
		    if ! _contains $sub $res
		    then
			res="$res $sub"
		    fi
		done
	    fi
	done
	if ! _contains $comp $res
	then
	    res="$res $comp"
	fi
    done

    echo $res
}

component_sources()
{
    (
	local comp
	
	for comp in $@
	do
	    echo "${COMPONENTS_DIR}/$comp.comp"
	    ( COMP_SOURCES=""; source ${COMPONENTS_DIR}/$comp.comp && echo $COMP_SOURCES )
	done
    ) | _unique_list
}

component_transitive_sources()
{
    component_sources `component_transitive_closure "$@"`
}

component_modified()
{
    vc_changed_revision `component_sources "$@"`
}

component_transitive_modified()
{
    vc_changed_revision `component_transitive_sources "$@"`
}

component_cache_modified()
{
    if ! vc_is_available ; then
        # Fake it if we can
        vc_changed_revision "$1"
    else
	local version=`echo $1 | sed "s:$2:\1:"`
	local revision=`echo $version | sed 's/\./ /g' | awk '{print $2;}'`
	echo $revision
    fi
}

component_external_path()
{
    local varname="BUILD_EXTERNAL_`echo $1 | sed 's/-/_/g' | tr '[:lower:]' '[:upper:]'`"
    eval "echo \$${varname}"
}

component_is_external()
{
    [ -n "`component_external_path $1`" ]
}

component_is_tool()
{
    local tool=`( COMP_IS_TOOL=""; source "${BUILD_ROOT}/src/linux/build/components/${1}.comp" && echo $COMP_IS_TOOL )`
    [ -n "${tool}" ]
}

# Cannot use 'seq' program as not all platforms have it.
_gen_indices()
{
    local count=$1
    local i=0
    local seq=""
    while [ $i -lt $count ]; do
        seq="${seq} ${i}"
        let i=i+1
    done
    echo "${seq}"
}

component_option_names()
{
    ( COMP_OPTIONS=() ; source "${BUILD_ROOT}/src/linux/build/components/${1}.comp" && for i in $(_gen_indices ${#COMP_OPTIONS[@]}) ; do ( echo "${COMP_OPTIONS[$i]}" | awk '{print $1}' ); done )
}

component_option_help()
{
    local comp="$1"
    local opt="$2"
    (
        set -e
        COMP_OPTIONS=()
        source "${BUILD_ROOT}/src/linux/build/components/${comp}.comp"
        for i in $(_gen_indices ${#COMP_OPTIONS[@]}) ; do
            if [ `echo "${COMP_OPTIONS[$i]}" | awk '{print $1}'` = "${opt}" ]; then
                echo "${COMP_OPTIONS[$i]}" | awk 'BEGIN { FS="[\t]+" } { print $2 }'
            fi
        done
    )
}
