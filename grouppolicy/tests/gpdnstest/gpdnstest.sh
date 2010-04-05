#!/bin/bash

LIBDIR=`rpm -E "%_lib"`
if [ -z "${LIBDIR}" ]; then
    LIBDIR=lib
fi

PKG=rpm
PKGDIR=RPM
BLDDIR=package/BUILD
VER=3.5.0
OPTDIR=opt

case "${BUILD_OS_TYPE}" in
    linux)
        OPTDIR=usr
        ;;
    solaris)
        PKG=pkg
        PKGDIR=PKG
        BLDDIR=building
        ;;
esac

STGPREFIX="${OPTDIR}/centeris"
case "${BUILD_OS_TYPE}" in
    solaris)
        STGPREFIX="prefix"
        ;;
esac

function getpkgname
{
    case "$1" in
        krb5)
            SNAME=Krb5
            ;;
        grouppolicy)
            SNAME=GroupPolicy
            ;;
        openldap)
            SNAME=OpenLDAP
            ;;
        libxml2)
            SNAME=LibXML2
            ;;
        *)
            echo "UNKNOWN"
            exit 1
            ;;
    esac
    SNAME=Centeris${SNAME}
    LNAME=centeris-$1

    case "${BUILD_OS_TYPE}" in
        solaris|hpux)
            echo "$SNAME"
            ;;
        *)
            echo "$LNAME"
            ;;
    esac
}

function getbld
{
    local p=`getpkgname $1`
    echo "${BUILD_META_OS_ROOT}/${PKGDIR}/$p/${BLDDIR}/$p-${VER}"
}

function getstaging
{
    local p=`getpkgname $1`
    echo "${BUILD_META_OS_ROOT}/${PKGDIR}/$p/staging/${STGPREFIX}"
}

function getlib
{
    echo "`getstaging $1`/${LIBDIR}"
}

function main
{
    case "$1" in
        b)
            mkpkg --nocompat --copy ${PKG} krb5 && \
                mkpkg --nocompat --copy ${PKG} openldap && \
                mkpkg --nocompat --copy ${PKG} libxml2 && \
                mkpkg --nocompat ${PKG} grouppolicy
            return $?
            ;;
        d|r)
            DBGCMD=""
            if [ "$1" = 'd' ]; then
                DBGCMD="gdb --args"
            fi
            shift
            LD_LIBRARY_PATH=`getlib krb5`:`getlib openldap`:`getlib grouppolicy` \
                ${DBGCMD} \
                "`getbld grouppolicy`/tests/gpdnstest/gpdnstest" "$@"
            return $?
            ;;
        *)
            echo usage...
            return 1
            ;;
    esac
}

main "$@"
exit $?
