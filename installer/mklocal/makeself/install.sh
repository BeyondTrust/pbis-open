#!/bin/sh
# ex: set tabstop=4 expandtab shiftwidth=4:

#
# Copyright (c) BeyondTrust Software.  All rights reserved.
#

ERR_PACKAGE_FILE_NOT_FOUND=1
ERR_PACKAGE_COULD_NOT_INSTALL=2
ERR_PACKAGE_NOT_INSTALLED=3
ERR_PACKAGE_COULD_NOT_UNINSTALL=4

log_info()
{
    echo "$@" 1>&2
}

# Expects one file name
escape()
{
    files=`echo "$@" | sed 's/ /\\ /g'`
    echo "$files"
}

setup_os_vars()
{
    OS_TYPE=`uname`
    exit_on_error $? "Could not determine OS type"
    OS_ARCH_COMMAND="uname -p"

    id=/usr/bin/id
    # if /usr/xpg4/bin/id exists, it more likely to support -u
    if [ -x /usr/xpg4/bin/id ]; then
        id=/usr/xpg4/bin/id
    fi

    # Check whether it supports -u
    "$id" -u >/dev/null 2>&1
    if [ $? -ne 0 ]; then
        id=perl_uid
    fi

    #
    # Interesting RPM install options:
    #
    # --force = --replacepkgs + --oldpackage + --replacefiles
    #
    # --replacepkgs = installs packages even if already installed
    # --oldpackage = allow installing older versions
    # --replacefiles = replaces already installed files
    #

    RPM_INSTALL_OPTIONS="-Uvh --replacepkgs"

    #
    # Interesting dpkg install options:
    #
    # --refuse-depends = make sure that we do not install w/o deps.
    # --force-confnew = make sure we replace modified conf files
    #   including any self-modifying init scripts.
    #

    DPKG_INSTALL_OPTIONS="-i --refuse-depends --force-confnew"

    case "${OS_TYPE}" in
        AIX)
            OS_TYPE='aix'
            PKGTYPE="aix_bff"
            ;;
        Linux)
            OS_TYPE='linux'
            # Use -m because it works on old Linux distros
            OS_ARCH_COMMAND="uname -m"
            if [ -f /etc/debian_version ]; then
                PKGTYPE="linux_deb"
            else
                PKGTYPE="linux_rpm"
            fi
            ;;
        FreeBSD)
            OS_TYPE='freebsd'
            PKGTYPE='freebsd'
            ;;
        SunOS)
            OS_TYPE='solaris'
            PKGTYPE='solaris'
            ;;
        Darwin)
            OS_TYPE='darwin'
            PKGTYPE='darwin'
            ;;
        HP-UX)
            OS_TYPE='hpux'
            OS_ARCH_COMMAND="getconf CPU_VERSION"
            PKGTYPE='hpux'
            ;;
        *)
            exit_on_error 1 "OS type \"${OS_TYPE}\" is not supported"
            ;;
    esac

    OS_ARCH=`$OS_ARCH_COMMAND`
    exit_on_error $? "Could not determine CPU architecture for OS \"${OS_TYPE}\""

    # Fix up OS_ARCH based on OS
    case "${OS_TYPE}" in
        linux)
            case "${OS_ARCH}" in
                i386|i486|i586|i686)
                    OS_ARCH=i386
                    ;;
            esac
            ;;
        hpux)
            case "${OS_ARCH}" in
                768)
                    OS_ARCH=ia64
                    ;;
                532)
                    OS_ARCH=hppa20
                    ;;
            esac
            ;;
        freebsd)
            case "${OS_ARCH}" in
                amd64)
                    OS_ARCH=x86_64
                    ;;
            esac
            ;;
    esac
}

exit_on_error()
{
    if [ $1 -ne 0 ]; then
        if [ -n "$2" ]; then
            log_info "ERROR: $2" 1>&2
        fi
        exit $1
    fi
}

perl_uid()
{
    /usr/bin/perl -e 'print "$>\n"'
}

do_setup()
{
    umask 022

    if [ ! -d "${DIRNAME}/packages" ]; then
        log_info "ERROR: Directory ${DIRNAME}/packages is not present."
        log_info "       Perhaps you need to use the --dir option."
        exit 1
    fi

    ## initial package list variables
    . "${DIRNAME}/MANIFEST"

    if [ `"$id" -u` != 0 ]; then
        log_info "ERROR: Root privileges are required to install this software. Try running this installer with su or sudo."
        exit 1
    fi

    PKGDIR_RELATIVE="packages"
    PKGDIR="${DIRNAME}/${PKGDIR_RELATIVE}"
    if [ ! -d "${PKGDIR}" ]; then
        exit_on_error 1 "The installer does not support this OS (${OS_TYPE}) and architecture (${OS_ARCH})."
    fi

    check_specific_os

    libdir=/opt/pbis/lib
    if [ -x /opt/pbis/lib64 ]; then
        libdir=/opt/pbis/lib64
    fi
    for i in "$LD_LIBRARY_PATH" "$LIBPATH" "$SHLIB_PATH"; do
        if [ -n "$i" ]; then
            expr "$i" : "^$libdir:" >/dev/null
            if [ $? -ne 0 ]; then
                exit_on_error 1 "LD_LIBRARY_PATH, LIBPATH, and SHLIB_PATH must be unset or list $libdir as the first directory. See the \"Requirements for the Agent\" section of the PowerBroker Identity Services manual for more information."
            fi
        fi
    done
    for i in "$LD_PRELOAD"; do
        if [ -n "$i" ]; then
            exit_on_error 1 "LD_PRELOAD must be unset. See the \"Requirements for the Agent\" section of the PowerBroker Identity Services manual for more information."
        fi
    done
}

check_specific_os_freebsd()
{
    if [ "$BUILD_UNAME" != 'FreeBSD' ]; then
        return 0
    fi

    BUILD_UNAME_R_MAJOR=`echo $BUILD_UNAME_R | grep -o "^[0-9]\+*\." | grep -o "^[0-9]\+"`
    CURRENT_UNAME_R_MAJOR=`uname -r | grep -o "^[0-9]\+*\." | grep -o "^[0-9]\+"`

    if [ "`uname`" != 'FreeBSD' ]; then
        echo "System is not FreeBSD"
        return 1
    fi

    if [ "$CURRENT_UNAME_R_MAJOR" != "$BUILD_UNAME_R_MAJOR" ]; then
        echo "System is not FreeBSD ${BUILD_UNAME_R_MAJOR}.x"
        return 1
    fi

    return 0
}

check_specific_os()
{
    Message=''

    if [ -n "$OPT_IGNORE_SPECIFIC_OS" ]; then
        return 0
    fi

    case "${OS_TYPE}" in
        freebsd)
            Message=`check_specific_os_freebsd`
        ;;
    esac

    if [ -n "$Message" ]; then
       log_info "Error: $Message"
       log_info "Use --ignore-specific-os to avoid this error, but you may break your system."
       exit 1
   fi
}

package_file_exists_aix_bff()
{
    pkgFile=${PKGDIR}/$1-*.bff
    if [ -f $pkgFile ]; then
        echo "$pkgFile"
        return 0
    fi
    return $ERR_PACKAGE_FILE_NOT_FOUND
}

is_package_installed_aix_bff()
{
    lslpp -L $1 > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo $1
        return 0
    fi
    return $ERR_PACKAGE_NOT_INSTALLED
}

package_install_aix_bff()
{
    geninstall -I "aX" -d $@ I:all
    if [ $? -eq 0 ]; then
        return 0
    fi
    return $ERR_PACKAGE_COULD_NOT_INSTALL
}

package_uninstall_aix_bff()
{
    geninstall -u $@
    if [ $? -eq 0 ]; then
        return 0
    fi
    return $ERR_PACKAGE_COULD_NOT_UNINSTALL
}

package_purge_aix_bff()
{
    geninstall -u $@
    if [ $? -eq 0 ]; then
        return 0
    fi
    return $ERR_PACKAGE_COULD_NOT_UNINSTALL
}

package_file_exists_aix_rpm()
{
    pkgFile=${PKGDIR}/$1_*.rpm
    if [ -f $pkgFile ]; then
        pkgFileBasename=`basename "${pkgFile}"`
        echo "R:`basename ${pkgFileBasename}`"
        return 0
    fi
    return $ERR_PACKAGE_FILE_NOT_FOUND
}

is_package_installed_aix_rpm()
{
    lslpp -L $1 > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo $1
        return 0
    fi
    return $ERR_PACKAGE_NOT_INSTALLED
}

package_install_aix_rpm()
{
    geninstall -d "${PKGDIR}" -F $@
    if [ $? -eq 0 ]; then
        return 0
    fi
    return $ERR_PACKAGE_COULD_NOT_INSTALL
}

package_uninstall_aix_rpm()
{
    geninstall -u $@
    if [ $? -eq 0 ]; then
        return 0
    fi
    return $ERR_PACKAGE_COULD_NOT_UNINSTALL
}

package_purge_aix_rpm()
{
    geninstall -u $@
    if [ $? -eq 0 ]; then
        return 0
    fi
    return $ERR_PACKAGE_COULD_NOT_UNINSTALL
}

package_file_exists_freebsd()
{
    pkg_file=${PKGDIR}/$1-[0-9]*.tbz
    if [ -f $pkg_file ]; then
        echo "$pkg_file"
        return 0
    fi
    return $ERR_PACKAGE_FILE_NOT_FOUND
}

is_package_installed_freebsd()
{
    pkgName=`pkg_info -E "$1-[0-9]*" 2>/dev/null`
    if [ $? -eq 0 ]; then
        echo "$pkgName"
        return 0
    fi
    return $ERR_PACKAGE_NOT_INSTALLED
}

package_install_freebsd()
{
    pkg_add $@ > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        pkgName=`basename $@ | sed -e 's/-x86_64\.tbz$//' | sed -e 's/-x86\.tbz$//'`
        # We don't want to be updated by the port system (bug 11833)
        touch "/var/db/pkg/$pkgName/+IGNOREME" >/dev/null 2>&1
        return 0
    fi
    return $ERR_PACKAGE_NOT_INSTALLED
}

package_uninstall_freebsd()
{
    pkg_delete $@ >/dev/null 2>&1
    if [ $? -eq 0 ]; then
        return 0
    fi
    return $ERR_PACKAGE_COULD_NOT_UNINSTALL
}

package_purge_freebsd()
{
    pkg_delete $@ >/dev/null 2>&1
    if [ $? -eq 0 ]; then
        return 0
    fi
    return $ERR_PACKAGE_COULD_NOT_UNINSTALL
}

package_file_exists_hpux()
{
    pkgFile=`(cd "${PKGDIR}" && echo $1-[0-9]*.depot)`
    pkgFile="${PKGDIR}/${pkgFile}"
    if [ -f "${pkgFile}" ]; then
        echo "$pkgFile"
        return 0
    fi
    return $ERR_PACKAGE_FILE_NOT_FOUND
}

is_package_installed_hpux()
{
    # The tab after the package name is important because some of our package
    # names are a prefix of other package names.
    swlist -l subproduct | grep " $1	" >/dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo "$1"
        return 0
    fi
    return $ERR_PACKAGE_NOT_INSTALLED
}

package_install_hpux()
{
    eval "swinstall -x match_target=true -x mount_all_filesystems=false -s $@ \*"
    if [ $? -eq 0 ]; then
        return 0
    fi
    return $ERR_PACKAGE_COULD_NOT_INSTALL
}

package_uninstall_hpux()
{
    eval "swremove -x mount_all_filesystems=false $@"
    return 0
}

package_purge_hpux()
{
    eval "swremove -x mount_all_filesystems=false $@"
    return 0
}

package_file_exists_linux_rpm()
{
    pkgFile=`(cd "${PKGDIR}" && echo $1-[0-9]*.rpm)`
    pkgFile="${PKGDIR}/${pkgFile}"
    if [ -f "${pkgFile}" ]; then
        echo "$pkgFile"
        return 0
    fi
    return $ERR_PACKAGE_FILE_NOT_FOUND
}

is_package_installed_linux_rpm()
{
    rpm -q $1 > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo "$1"
        return 0
    fi
    return $ERR_PACKAGE_NOT_INSTALLED
}

package_install_linux_rpm()
{
    rpm ${RPM_INSTALL_OPTIONS} "'$@'"
    if [ $? -eq 0 ]; then
        return 0
    fi
    return $ERR_PACKAGE_COULD_NOT_INSTALL
}

package_uninstall_linux_rpm()
{
    eval "rpm -e --nodeps $@"
    if [ $? -eq 0 ]; then
        return 0
    fi
    return $ERR_PACKAGE_COULD_NOT_INSTALL
}

package_purge_linux_rpm()
{
    eval "rpm -e $@"
    if [ $? -eq 0 ]; then
        return 0
    fi
    return $ERR_PACKAGE_COULD_NOT_INSTALL
}

package_file_exists_linux_deb()
{
    pkgFile=`(cd "${PKGDIR}" && echo $1_*.deb)`
    pkgFile="${PKGDIR}/${pkgFile}"
    if [ -f "$pkgFile" ]; then
        echo "$pkgFile"
        return 0
    fi
    return $ERR_PACKAGE_FILE_NOT_FOUND
}

is_package_installed_linux_deb()
{
    _status="`dpkg -s $1 2>/dev/null | grep Status: 2>/dev/null`"
    if [ $? -eq 0 ]
    then
        if echo "$_status" | grep ' installed' >/dev/null 2>&1
        then
            echo $1
            return 0
        fi
    fi

    return 1
}

is_package_uninstalled_linux_deb()
{
    _status="`dpkg -s $1 2>/dev/null | grep Status: 2>/dev/null`"
    if [ $? -eq 0 ]
    then
        if echo "$_status" | grep ' deinstall' >/dev/null 2>&1
        then
            echo $1
            return 0
        fi
    fi

    return 1
}

package_install_linux_deb()
{
    eval "dpkg ${DPKG_INSTALL_OPTIONS} '$@'"
    if [ $? -eq 0 ]; then
        return 0;
    fi
    return $ERR_PACKAGE_COULD_NOT_INSTALL
}

package_uninstall_linux_deb()
{
    eval "dpkg -r --force-depends $@"
    if [ $? -eq 0 ]; then
        return 0
    fi

    return $ERR_PACKAGE_COULD_NOT_UNINSTALL
}

package_purge_linux_deb()
{
    eval "dpkg --purge $@"
    if [ $? -eq 0 ]; then
        return 0
    fi

    return $ERR_PACKAGE_COULD_NOT_UNINSTALL
}

package_file_exists_solaris()
{
    pkgFile=`(cd "${PKGDIR}" && echo $1-[0-9]*.pkg)`
    pkgFile="${PKGDIR}/${pkgFile}"
    if [ -f "$pkgFile" ]; then
        echo "$pkgFile"
        return 0
    fi
    return $ERR_PACKAGE_FILE_NOT_FOUND
}

is_package_installed_solaris()
{
    pkginfo $1 > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo $1
        return 0
    fi
    return $ERR_PACKAGE_NOT_FOUND
}

package_install_solaris()
{
    EXTRA_OPTIONS=""

    if [ "${OPT_SOLARIS_CURRENT_ZONE}" = "yes" ]; then
        EXTRA_OPTIONS="$EXTRA_OPTIONS -G"
    fi

    pkgList=`eval echo "$@"`
    for pkgFile in $pkgList; do
        eval "pkgadd ${EXTRA_OPTIONS} -a ${DIRNAME}/response -d $pkgFile all"
        err=$?
        if [ $err -eq 1 ]; then
            return $ERR_PACKAGE_COULD_NOT_INSTALL
        fi
    done
    return 0
}

package_uninstall_solaris()
{
    if [ -f "/var/lib/pbis/uninstall/response" ]; then
        RESPONSE="-a /var/lib/pbis/uninstall/response"
    else
        RESPONSE="-a ${DIRNAME}/response"
    fi

    pkgList=`eval echo "$@"`
    for pkg in $pkgList; do
        pkgrm $RESPONSE -n "$pkg"
        err=$?
        if [ $err -eq 1 ]; then
            return $ERR_PACKAGE_COULD_NOT_UNINSTALL
        fi
    done
    return 0
}

package_purge_solaris()
{
    if [ -f "/var/lib/pbis/uninstall/response" ]; then
        RESPONSE="-a /var/lib/pbis/uninstall/response"
    else
        RESPONSE="-a ${DIRNAME}/response"
    fi

    pkgList=`eval echo "$@"`
    for pkg in $pkgList; do
        pkgrm $RESPONSE -n "$pkg"
        err=$?
        if [ $err -eq 1 ]; then
            return $ERR_PACKAGE_COULD_NOT_UNINSTALL
        fi
    done
    return 0
}

remove_extra_files()
{
    for file in /opt/likewise /etc/likewise /var/log/likewise /var/lib/likewise /var/cache/likewise /opt/pbis /etc/pbis /var/log/pbis /var/lib/pbis /var/cache/pbis ; do
        if [ -d "$file" ]; then
            echo "Removing directory $file"
            /bin/rm -rf "$file"
        fi
    done

    echo "Remove PowerBroker Identity Services created backup/restore files"
    for file in /etc/pam.conf /etc/pam.d/* /etc/krb5.conf /etc/krb5/* /etc/hosts /etc/sshd_config /etc/ssh_config /etc/ssh/* /etc/nsswitch.conf /etc/skel /etc/inet/* /etc/hostname.* /etc/defaultdomain /usr/lib/security/methods.cfg /etc/security/user /etc/security/login.cfg /etc/netsvc.conf /etc/methods.cfg; do
        orig="$file.lwidentity.orig"
        bak="$file.lwidentity.bak"
        if [ -f "$orig" ]; then
            rm -f $orig
        fi
        if [ -f "$bak" ]; then
            rm -f $bak
        fi
    done

    return 0
}

is_package_installed()
{
    is_package_installed_${PKGTYPE} "$@"
    return $?
}

is_package_uninstalled()
{
    is_package_uninstalled_linux_deb "$@"
    return $?
}

package_file_exists()
{
    pkgName=`package_file_exists_${PKGTYPE} "$@"`
    err=$?
    if [ $err -eq 0 ]; then
        escape $pkgName
        return $?
    else
        return $err
    fi
}

package_install()
{
    package_install_${PKGTYPE} "$@"
    return $?
}

package_uninstall()
{
    package_uninstall_${PKGTYPE} "$@"
    return $?
}

package_purge()
{
    package_purge_${PKGTYPE} "$@"
    return $?
}

remove_likewise_directories()
{
    if [ -d "/opt/likewise" ]
    then
        (
            IFS='
'
            for file in `find /opt/likewise`
            do
                if [ -d "${file}" ]
                then
                    echo "${file}"
                fi
            done
        ) | sort -r | xargs rmdir >/dev/null 2>&1
    fi
}

do_install()
{
    log_info "Installing packages and old packages will be removed"

    # Install upgrade helper package.
    if [ -n "$INSTALL_UPGRADE_PACKAGE" ]; then
        pkgName=`is_package_installed $INSTALL_UPGRADE_PACKAGE`
        if [ $? -eq 0 ]; then
            package_uninstall "$pkgName"
            err=$?
            if [ $err -ne 0 ]; then
                log_info "Error uninstalling $pkgName"
                exit 1
            fi
        fi

        pkgName=`package_file_exists $INSTALL_UPGRADE_PACKAGE`
        if [ $? -eq 0 ]; then
            package_install "$pkgName"
            err=$?
            if [ $err -ne 0 ]; then
                log_info "Error installing $pkgName"
                exit 1
            fi
        else
            log_info "Missing package file for $INSTALL_UPGRADE_PACKAGE"
        fi
    fi

    # Uninstall all old packages in one call to ensure interpackage dependencies
    # do not cause a failure.
    pkgList=""
    for pkg in $INSTALL_OBSOLETE_PACKAGES
    do
        pkgName=`is_package_installed $pkg`
        if [ $? -eq 0 ]; then
            if [ -n "${pkgList}" ]; then
                pkgList="${pkgList} $pkgName"
            else
                pkgList="$pkgName"
            fi
        fi
    done

    if [ -n "$pkgList" ]; then
        package_uninstall "$pkgList"
        err=$?
        if [ $err -ne 0 ]; then
            log_info "Error uninstalling obsolete packages $pkgList"
            exit 1
        fi
    fi

    # Some of the earlier packages didn't delete their directories and this was
    # masked by the install script cleaning up after them. So we have to
    # continue the tradition.
    remove_likewise_directories


    # Install base package.
    if [ -n "$INSTALL_BASE_PACKAGE" ]; then
        pkgName=`package_file_exists $INSTALL_BASE_PACKAGE`
        if [ $? -eq 0 ]; then
            package_install "$pkgName"
            err=$?
            if [ $err -ne 0 ]; then
                log_info "Error installing $pkgName"
                exit 1
            fi
        else
            log_info "Missing package file for $INSTALL_BASE_PACKAGE"
            exit 1
        fi
    fi

    if [ -n "$INSTALL_GUI_PACKAGE" ]; then
        pkgName=`package_file_exists $INSTALL_GUI_PACKAGE`
        if [ $? -eq 0 ]; then
            package_install "$pkgName"
            err=$?
            if [ $err -ne 0 ]; then
                log_info "Error installing $pkgName"
            fi
        else
            log_info "Missing package file $pkgName for $INSTALL_GUI_PACKAGE"
        fi
    fi

    if [ -n "$INSTALL_LEGACY_PACKAGE" ]; then
        if [ -z "$OPT_INSTALL_LEGACY_PACKAGE" -a -f "/var/lib/pbis-upgrade/VERSION" ]; then
            DO_INSTALL_LEGACY_PACKAGE="yes"
        fi
        if [ "$OPT_INSTALL_LEGACY_PACKAGE" = "yes" ]; then
            DO_INSTALL_LEGACY_PACKAGE="yes"
        fi

        if [ "$DO_INSTALL_LEGACY_PACKAGE" = "yes" ]; then
            pkgName=`package_file_exists $INSTALL_LEGACY_PACKAGE`
            if [ $? -eq 0 ]; then
                package_install "$pkgName"
               err=$?
                if [ $err -ne 0 ]; then
                    log_info "Error installing $pkgName"
                fi
            else
                log_info "Missing package file $pkgName for $INSTALL_LEGACY_PACKAGE"
            fi
        fi
    fi

    mkdir -p /var/lib/pbis/uninstall
    echo "PREFIX=\"$PREFIX\"" > /var/lib/pbis/uninstall/MANIFEST
    echo "PKGTYPE=\"$PKGTYPE\"" >> /var/lib/pbis/uninstall/MANIFEST
    echo "INSTALL_UPGRADE_PACKAGE=\"$INSTALL_UPGRADE_PACKAGE\"" >> /var/lib/pbis/uninstall/MANIFEST
    echo "INSTALL_BASE_PACKAGE=\"$INSTALL_BASE_PACKAGE\"" >> /var/lib/pbis/uninstall/MANIFEST
    echo "INSTALL_GUI_PACKAGE=\"$INSTALL_GUI_PACKAGE\"" >> /var/lib/pbis/uninstall/MANIFEST
    echo "INSTALL_LEGACY_PACKAGE=\"$INSTALL_LEGACY_PACKAGE\"" >> /var/lib/pbis/uninstall/MANIFEST
    if [ -f "${DIRNAME}/response" ]; then
        cp "${DIRNAME}/response" /var/lib/pbis/uninstall/response
    fi

    log_info "Installing Packages was successful"
}

do_postinstall_messages()
{
    RUN_JOIN_GUI="1"
    guimsg=""

    if [ "$1" != 'interactive' ]; then
        RUN_JOIN_GUI=""
    fi

    if [ -x "/opt/pbis/bin/domainjoin-gui" ]; then
        guimsg="domainjoin-gui or "
    else
        RUN_JOIN_GUI=""
    fi

    if [ -n "$OPT_DONT_JOIN" ]; then
        RUN_JOIN_GUI=""
    fi

    domain=`/opt/pbis/bin/lsa ad-get-machine account 2>/dev/null | grep '  DNS Domain Name: ' | sed -e 's/  DNS Domain Name: //'`

    if [ -n "$domain" ]; then
        log_info ""
        log_info "This computer is joined to $domain"
    fi

    log_info ""
    log_info "New libraries and configurations have been installed for PAM and NSS."
    log_info "Please reboot so that all processes pick up the new versions."
    log_info ""

    if [ -z "$domain" ]; then
        log_info "As root, run ${guimsg}domainjoin-cli to join a domain so you can log on"
        log_info "with Active Directory credentials. Example:"
        log_info "domainjoin-cli join MYDOMAIN.COM MyJoinAccount"
        log_info ""

        if [ -n "$RUN_JOIN_GUI" ]; then
            /opt/pbis/bin/domainjoin-gui >/dev/null 2>&1 &
        fi
    fi
}

scrub_prefix()
{
    if [ -d "${PREFIX}" ]
    then
        (
            IFS='
'
            for file in `find ${PREFIX}`
            do
                if [ -d "${file}" ]
                then
                    echo "${file}"
                fi
            done
            ) | sort -r | xargs rmdir >/dev/null 2>&1
    fi
}

do_uninstall()
{
    log_info "Uninstalling packages"

    if [ "$OS_TYPE" = 'solaris' ]; then
        UNINSTALL_EXTRA_PACKAGES="PBISopenu PBISopenr"
    fi

    pkgList=""
    for pkg in $INSTALL_OBSOLETE_PACKAGES $INSTALL_UPGRADE_PACKAGE $INSTALL_GUI_PACKAGE $INSTALL_LEGACY_PACKAGE $UNINSTALL_EXTRA_PACKAGES $INSTALL_BASE_PACKAGE;
    do
        pkgName=`is_package_installed $pkg`
        if [ $? -eq 0 ]; then
            pkgList="$pkgList $pkgName"
        fi
    done

    if [ -n "$pkgList" ]; then
        package_uninstall $pkgList
        err=$?
        if [ $err -ne 0 ]; then
            log_info "Error uninstalling packages $pkgList"
        fi
    fi

    scrub_prefix
}

do_purge()
{
    log_info "Uninstalling packages and purging data files"

    domainjoin_cli=/opt/likewise/bin/domainjoin-cli
    if [ -x "$domainjoin_cli" ]; then
        $domainjoin_cli leave > /dev/null 2>&1
    else
        domainjoin_cli=/opt/pbis/bin/domainjoin-cli
        if [ -x "$domainjoin_cli" ]; then
            $domainjoin_cli leave > /dev/null 2>&1
        fi
    fi

    if [ "$OS_TYPE" = 'solaris' ]; then
        UNINSTALL_EXTRA_PACKAGES="PBISopenu PBISopenr"
    fi

    pkgList=""
    for pkg in $INSTALL_OBSOLETE_PACKAGES $INSTALL_UPGRADE_PACKAGE $INSTALL_GUI_PACKAGE $INSTALL_LEGACY_PACKAGE $UNINSTALL_EXTRA_PACKAGES $INSTALL_BASE_PACKAGE;
    do
        pkgName=`is_package_installed $pkg`
        if [ $? -eq 0 ]; then
            pkgList="$pkgList $pkgName"
        else
            pkgName=`is_package_uninstalled $pkg`
            if [ $? -eq 0 ];then
                pkgList="$pkgList $pkgName"
            fi
        fi
    done

    if [ -n "$pkgList" ]; then
        package_purge $pkgList
        err=$?
        if [ $err -ne 0 ]; then
            log_info "Error uninstalling packages $pkgList"
        fi
    fi

    remove_extra_files

    scrub_prefix

    log_info "Uninstall complete"
}

do_info()
{
    ## install.sh is always called from installers with a relative path
    echo ""
    echo "To install at a later time, please run `pwd`/`basename $0` install"
    echo ""

    exit 0
}

prompt_yes_no()
{
    _prompt="$1"
    _auto_ok="$2"
    _msgfunc="$3"

    _allowed="yes/no"
    _default=""

    if [ -n "${_auto_ok}" ]; then
        _allowed="auto/yes/no"
        _default="auto"
    fi

    answer=""
    until test -n "${answer}" ; do

        if [ -n "${_msgfunc}" ]; then
            ${_msgfunc}
        fi

        # Solaris has issues with echo -n, so the tr turns the trailing
        # newline into a space, thus emulating echo -n

        echo "${_prompt} (${_allowed})" | tr '\n' ' '
        read _answer

        _answer=`echo ${_answer} | tr [A-Z] [a-z]`
        case "${_answer}" in
            y|ye|yes)
                answer=yes
                ;;
            n|no)
                answer=no
                ;;
            a|au|aut|auto)
                if [ -n "${_auto_ok}" ]; then
                    answer=auto
                fi
                ;;
            '')
                if [ -n "${_default}" ]; then
                    answer="${_default}"
                fi
                ;;
        esac
    done
}

do_interactive()
{
    if [ "x${PAGER}" = "x" ]; then
        PAGER=more
    fi

    if [ -f "${DIRNAME}/EULA" ]; then
        cat "${DIRNAME}/EULA" | ${PAGER}
        prompt_yes_no "Do you accept the terms of these licenses?"
        if [ "x$answer" != "xyes" ]; then
            echo "License not accepted."
            exit 1
        fi

        echo ""
        echo "License accepted."
        echo ""
    fi

    prompt_yes_no "Would you like to install package for legacy links? (i.e.  /opt/likewise/bin/lw-find-user-by-name -> /opt/pbis/bin/find-user-by-name)"
    if [ "x$answer" = "xyes" ]; then
        OPT_INSTALL_LEGACY_PACKAGE="yes"
    elif [ "x$answer" = "xno" ]; then
        OPT_INSTALL_LEGACY_PACKAGE="no"
    elif [ "x$answer" = "xauto" ]; then
        OPT_INSTALL_LEGACY_PACKAGE=""
    fi

    prompt_yes_no "Would you like to install now?"
    if [ "x$answer" != "xyes" ]; then
        do_info
        exit 0
    fi
}

# must check before shift to avoid shift error on some sh versions.
check_arg_present()
{
    if [ -z "$1" ]; then
        if [ -n "$2" ]; then
            echo "Missing argument for $2"
        fi
        usage
        exit 1
    fi
}

usage()
{
    echo "usage: install.sh [options] [command]"
    echo ""
    echo "  where options:"
    echo ""
    echo "    --dir <DIR>      base directory where this script is located"
    echo "    --echo-dir <DIR> prefix to output for packages directory (w/info command)"
    echo "    --dont-join      do not run the domainjoin GUI tool after install completes (default: auto)"
    echo "    --legacy         install the legacy package"
    echo "    --no-legacy      do not install the legacy package"

    if [ "${OS_TYPE}" = "solaris" ]; then
        echo "    --all-zones      install to all zones (default)"
        echo "    --current-zone   install only to the current zone"
    fi

    echo ""
    echo "  where command is one of:"
    echo ""
    echo "    install       silent install"
    echo "    uninstall     silent uninstall"
    echo "    purge         silent purge uninstall (same as uninstall but will unjoin domain"
    echo "                  and delete all generated files)"
    echo "    info          show commands to do a manual install"
    echo ""
    echo "  If not command is given, interactive mode is used."
    echo ""
}

main_install()
{
    OPT_DONT_JOIN=""
    OPT_SOLARIS_CURRENT_ZONE=""
    OPT_IGNORE_SPECIFIC_OS=""
    OPT_INSTALL_LEGACY_PACKAGE=""

    ECHO_DIRNAME=""

    parseOptDone=""
    while [ -z "$parseOptDone" ]; do
        case "$1" in
            --dir)
                check_arg_present "$2" "$1"
                DIRNAME="$2"
                shift 2
                ;;
            --echo-dir)
                check_arg_present "$2" "$1"
                ECHO_DIRNAME="$2"
                shift 2
                ;;
            --current-zone)
                if [ -n "${OPT_SOLARIS_CURRENT_ZONE}" ]; then
                    if [ "${OPT_SOLARIS_CURRENT_ZONE}" != "yes" ]; then
                        echo "Cannot use $1 with --all-zones"
                        usage
                        exit 1
                    fi
                fi
                OPT_SOLARIS_CURRENT_ZONE="yes"
                shift 1
                ;;
            --all-zones)
                if [ -n "${OPT_SOLARIS_CURRENT_ZONE}" ]; then
                    if [ "${OPT_SOLARIS_CURRENT_ZONE}" != "no" ]; then
                        echo "Cannot use $1 with --current-zone"
                        usage
                        exit 1
                    fi
                fi
                OPT_SOLARIS_CURRENT_ZONE="no"
                shift 1
                ;;
            --ignore-specific-os)
                OPT_IGNORE_SPECIFIC_OS="1"
                shift 1
                ;;
            --dont-join)
                OPT_DONT_JOIN="1"
                shift 1
                ;;
            --legacy)
                if [ -n "${OPT_INSTALL_LEGACY_PACKAGE}" ]; then
                    if [ "${OPT_INSTALL_LEGACY_PACKAGE}" != "yes" ]; then
                        echo "Cannot use $1 with --legacy"
                        usage
                        exit 1
                    fi
                fi
                OPT_INSTALL_LEGACY_PACKAGE="yes"
                shift 1
                ;;
            --no-legacy)
                if [ -n "${OPT_INSTALL_LEGACY_PACKAGE}" ]; then
                    if [ "${OPT_INSTALL_LEGACY_PACKAGE}" != "no" ]; then
                        echo "Cannot use $1 with --no-legacy"
                        usage
                        exit 1
                    fi
                fi
                OPT_INSTALL_LEGACY_PACKAGE="no"
                shift 1
                ;;
            *)
                parseOptDone=1
                ;;
        esac
    done

    VERB="$1"
    if [ -z "$VERB" ]; then
        VERB="interactive"
    else
        shift
    fi

    if [ -n "$1" ]; then
        usage
        exit 1
    fi

    case "${VERB}" in
        install)
            do_setup
            do_install
            do_postinstall_messages
            ;;
        uninstall)
            do_setup
            do_uninstall
            echo "Success"
            ;;
        purge)
            do_setup
            do_purge
            echo "Success"
            ;;
        info)
            do_setup
            do_info
            ;;
        interactive)
            do_setup
            do_interactive
            do_install
            do_postinstall_messages 'interactive'
            ;;
        help)
            usage
            exit 0
            ;;
        *)
            usage
            exit 1
            ;;
    esac

    return 0
}

usage_uninstall()
{
    echo "usage: uninstall.sh [command]"
    echo ""
    echo "  where command is one of:"
    echo ""
    echo "    uninstall     silent uninstall"
    echo "    purge         silent purge uninstall (same as uninstall but will unjoin domain"
    echo "                  and delete all generated files)"
    echo ""
}

main_uninstall()
{
    if [ -f /var/lib/pbis/uninstall/MANIFEST ]; then
        . /var/lib/pbis/uninstall/MANIFEST
    else
        echo "The file /var/lib/pbis/uninstall/MANIFEST cannot be found and"
        echo "is required for this uninstall procedure."
        exit 1
    fi

    VERB="$1"
    if [ -z "$VERB" ]; then
        VERB="help"
    else
        shift
    fi

    case "${VERB}" in
        help)
            usage_uninstall
            exit 0
            ;;
        uninstall)
            do_uninstall
            echo "Success"
            ;;
        purge)
            do_purge
            echo "Success"
            ;;
        *)
            usage_uninstall
            exit 1
            ;;
    esac
}

main()
{
    DIRNAME=`dirname $0`
    BASENAME=`basename $0`

    if [ -z "${DIRNAME}" ]; then
        DIRNAME=.
    fi

    if echo "${DIRNAME}" | grep -v '^/' >/dev/null 2>&1; then
        if [ "`pwd`" = '/' ]; then
            DIRNAME="/${DIRNAME}"
        else
            DIRNAME="`pwd`/${DIRNAME}"
        fi
    fi

    setup_os_vars

    if [ "$BASENAME" = "uninstall.sh" ]; then
        main_uninstall "$@"
    else
        main_install "$@"
    fi

    return 0
}

main "$@"
exit $?
