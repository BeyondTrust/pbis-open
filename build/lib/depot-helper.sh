#!/bin/bash

source "${BUILD_ROOT}/src/linux/build/lib/dep-helper.sh"

_depot_directory_nonempty()
{
    local path="${1}"
    local file

    for file in "${path}"/*
    do
        if [ -f "${file}" ]
        then
            return 0
        fi
    done

    return 1
}

_depot_directories()
{
    local path="${1}"
    local IFS=$'\n'
    local file

    for file in `find "${path}"/*`
    do
        if [ -d "${file}" ] && _depot_directory_nonempty "${file}"
        then
            echo "${file}"
        fi
    done
}

_depot_chomp_path()
{
    sed 's://*:/:g'
}

depot_generate_psf_fileset()
{
    local name="${1}"
    local destdir="${2}"
    local exclude="${3}"
    local dir

    cat <<HEADER
    fileset
        tag		    ${name}
        file_permissions    -o root -g root
HEADER

    for dir in `_depot_directories "${PKG_PACKAGE_ROOT}/${name}"`
    do
        if [ -z "${exclude}" ] || ! echo "${dir}" | grep "${exclude}" >/dev/null 2>&1
        then
            local source="`echo $dir | sed "s:^${PKG_PACKAGE_ROOT}/::" | _depot_chomp_path`"
            local dest="`echo $source | sed "s:^${name}/:${destdir}/:" | _depot_chomp_path`"
            local file
            cat <<DIR
        directory           ${source}=${dest}
DIR
            for file in "${dir}"/*
            do
                if [ -f "${file}" ]
                then
                    case ${file} in
                        *\.la | *\.a | *\.h)
                            # Ignore development files
                            ;;
                        *)
                            local base="`basename ${file}`"
                            cat <<FILE
        file                ${base}
FILE
                            ;;
                    esac
                fi
            done
        fi
    done

cat <<FOOTER
   end
FOOTER
}

depot_generate_psf()
{
    local pkg="${1}"
    local file="${2}"
    local pkgname="`package_native_name ${pkg}`"

    if [ -f "${PKG_DIR}/postinstall" ]; then
        postinstall="postinstall    ./postinstall"
    fi

    if [ -f "${PKG_DIR}/preinstall" ]; then
        preinstall="preinstall    ./preinstall"
    fi

    if [ -f "${PKG_DIR}/preremove" ]; then
        preremove="preremove    ./preremove"
    fi

    cat <<HEADER >"${file}"
product
    tag            ${pkgname}
    revision       ${PKG_VERSION}.${PKG_FULL_RELEASE}
    os_name        HP-UX
    os_release	   ?.11.??
    os_version	   *
    ${postinstall}
    ${preinstall}
    ${preremove}
HEADER

    depot_generate_psf_fileset prefix "${BUILD_PREFIX}" ""                >>"${file}"
    depot_generate_psf_fileset root   "/"               "${BUILD_PREFIX}" >>"${file}"

    cat <<FOOTER >>"${file}"
end
FOOTER

    set +x
}
