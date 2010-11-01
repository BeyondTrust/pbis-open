#!/bin/sh
# ex: set tabstop=4 expandtab shiftwidth=4:

#
# Copyright Likewise, 2006-2007, 2010.  All rights reserved.
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
        FreeBSD|"Isilon OneFS")
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
    setup_os_vars

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

    PKGDIR_RELATIVE="packages/${OS_TYPE}/${OS_ARCH}"
    PKGDIR="${DIRNAME}/${PKGDIR_RELATIVE}"
    if [ ! -d "${PKGDIR}" ]; then
        exit_on_error 1 "The installer does not support this OS (${OS_TYPE}) and architecture (${OS_ARCH})."
    fi

    HAVE_COMPAT=""
    if [ -d "${PKGDIR}/compat" ]; then
        HAVE_COMPAT=1
    fi

    NEED_COMPAT=""
    if [ "${OS_TYPE}" = "linux" ]; then
        if [ "${OS_ARCH}" = "x86_64" ]; then
            if [ "${PKGTYPE}" = "rpm" ]; then
                for pkg in pam pam-32bit ; do
                    PAM_ARCH_LIST=`get_rpm_arch ${pkg}`
                    for _arch in ${PAM_ARCH_LIST} ; do
                        case "${_arch}" in
                            i386|i486|i586|i686)
                                NEED_COMPAT=1
                                ;;
                        esac
                    done
                done
            fi
        fi
    fi

    update_packages_list "${OPT_COMPAT}"
}

update_packages_list()
{
    IS_COMPAT=""
    case "${1}" in
        yes)
            if [ -z "${HAVE_COMPAT}" ]; then
                exit_on_error 1 "The installer does not have 32-bit compatibility libraries for this platform."
            fi
            if [ -z "${NEED_COMPAT}" ]; then
                echo "WARNING: This system does not need 32-bit compatibility libraries, but the user"
                echo "         requested to install them."
            fi
            IS_COMPAT=1
            ;;
        no)
            if [ -n "${NEED_COMPAT}" ]; then
                echo "WARNING: This system needs 32-bit compatibility libraries, but the user"
                echo "         requested to not install them."
            fi
            IS_COMPAT=""
            ;;
        auto|'')
            if [ -n "${NEED_COMPAT}" -a -n "${HAVE_COMPAT}" ]
            then
                IS_COMPAT="${NEED_COMPAT}"
            fi
            ;;
        *)
            exit_on_error 1 "Invalid compatibility mode: \"$1\""
            ;;
    esac

    if [ -n "${IS_COMPAT}" ]; then
        if [ -z "${HAVE_COMPAT}" ]; then
            echo "${1}"
            exit_on_error 1 "INTERNAL: The installer is missing required 32-bit compatibility libraries in $PKGDIR/compat"
        fi
    fi
}

get_rpm_arch()
{
    _get_rpm_arch=`rpm -q --queryformat "%{ARCH}\n" $1`
    if [ $? -ne 0 ]; then
        return 1
    fi
    # potentially return multiple entries
    echo "${_get_rpm_arch}"
    return 0
}

package_file_exists_aix_bff()
{
    pkgFile=${PKGDIR}/$1-*.bff
    if [ -f $pkgFile ]; then
        echo "I:$1"
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
    return $ERR_PACKAGE_COULD_NOT_INSTALL
}

package_install_aix_bff()
{
    geninstall -I "aX" -d "${PKGDIR}" $@
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
    return $ERR_PACKAGE_COULD_NOT_INSTALL
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
        return 0
    fi
    return $ERR_PACKAGE_COULD_NOT_INSTALL
}

package_uninstall_freebsd()
{
    pkg_delete $@ >/dev/null 2>&1
    if [ $? -eq 0 ]; then
        return 0
    fi
    return $ERR_PACKAGE_COULD_NOT_UNINSTALL
}

is_package_installed_hpux()
{
    swlist -l subproduct | grep "$1" >/dev/null 2>&1
}

install_hpux()
{
    # Need to get the absolute path of the depot file

    for pkg in $@ ; do
        swinstall -x match_target=true -x mount_all_filesystems=false -s "${PKGDIR}/${pkg}"-*.depot \*
        exit_on_error $? "Failed to install package ${pkg}"
    done
    return 0
}

uninstall_hpux()
{
    _pkgs=""

    for pkg in $@; do
        check_depot_installed "$pkg" && _pkgs="$_pkgs $pkg"
    done

    if [ -n "$_pkgs" ]; then
        swremove -x mount_all_filesystems=false ${_pkgs}
        exit_on_error $? "Failed to uninstall packages ${_pkgs}"
    fi
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
    eval "rpm ${RPM_INSTALL_OPTIONS} $@"
    if [ $? -eq 0 ]; then
        return 0
    fi
    return $ERR_PACKAGE_COULD_NOT_INSTALL
}

package_uninstall_linux_rpm()
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
    _status="`dpkg -s "$1" 2>/dev/null | grep Status: 2>/dev/null`"
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

package_install_linux_deb()
{
    eval "dpkg ${DPKG_INSTALL_OPTIONS} $@"
    if [ $? -eq 0 ]; then
        return 0;
    fi
    return $ERR_PACKAGE_COULD_NOT_INSTALL
}

package_uninstall_linux_deb()
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
    pkgList=`eval echo "$@"`
    for pkgFile in $pkgList; do
        eval "pkgadd -a ${DIRNAME}/response -d $pkgFile all"
        err=$?
        if [ $err -ne 0 ]; then
            return $ERR_PACKAGE_COULD_NOT_INSTALL
        fi
    done
    return 0
}

package_uninstall_solaris()
{
    pkgList=`eval echo "$@"`
    for pkg in $pkgList; do
        eval "pkgrm -a ${DIRNAME}/response -n $pkg"
        err=$?
        if [ $err -ne 0 ]; then
            return $ERR_PACKAGE_COULD_NOT_UNINSTALL
        fi
    done
    return 0
}

remove_extra_files()
{
    for file in /opt/likewise /etc/likewise /var/log/likewise /var/lib/likewise /var/cache/likewise ; do
        if [ -d "$file" ]; then
            echo "Removing directory $file"
            /bin/rm -rf "$file"
        fi
    done

    echo "Remove Likewise created backup/restore files"
    for file in /etc/pam.conf /etc/pam.d/* /etc/krb5.conf /etc/krb5/* /etc/hosts /etc/sshd_config /etc/ssh_config /etc/ssh/* /etc/nsswitch.conf /etc/skel /etc/inet ; do
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

preinstall_pkgs()
{
    FIXUP_DIRS="/etc/samba /var/lib /var/log/lwidentity /var/cache/likewise"

    for _dir in ${FIXUP_DIRS}; do
        [ -d "$_dir" ] && chown -R root:other "$_dir"
    done

    echo "Creating base directory"
    mkdir -p "`get_prefix_dir`"
}

is_package_installed()
{
    is_package_installed_${PKGTYPE} "$@"
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

fix_old_registry()
{
    DomainSeparator=`${PREFIX}/bin/lwregshell list_values '[HKEY_THIS_MACHINE\Services\lsass\Parameters\Providers\ActiveDirectory]' | grep DomainSeparator | sed -e 's/ *[^ ]\+[ ]\+[^ ]\+[ ]\+"\([^ ]*\)"$/\1/'`
    SpaceReplacement=`${PREFIX}/bin/lwregshell list_values '[HKEY_THIS_MACHINE\Services\lsass\Parameters\Providers\ActiveDirectory]' | grep SpaceReplacement | sed -e 's/ *[^ ]\+[ ]\+[^ ]\+[ ]\+"\([^ ]*\)"$/\1/'`
    if [ -n "${DomainSeparator}" ]; then
        if [ "$DomainSeparator" = "\\\\" ]; then
            DomainSeparator="\\"
        fi
        ${PREFIX}/bin/lwregshell set_value '[HKEY_THIS_MACHINE\Services\lsass\Parameters]' 'DomainSeparator' "$DomainSeparator"
    fi
    if [ -n "${SpaceReplacement}" ]; then
        ${PREFIX}/bin/lwregshell set_value '[HKEY_THIS_MACHINE\Services\lsass\Parameters]' 'SpaceReplacement' "$SpaceReplacement"
    fi
}

# Fix registry settings that can't be upgraded automatically
fix_5_4_configuration()
{
    if [ -z "${UPGRADING_FROM_5_4}" ]; then
        return 0;
    fi

    switch_to_open_provider
}

get_prefix_dir()
{
    echo "${PREFIX}"
}

uninstall_darwin()
{
    # No easy way to uninstall individual packages on Mac OS X
    if [ -x /opt/likewise/bin/lwi-uninstall.sh ]; then
       /opt/likewise/bin/lwi-uninstall.sh

        if [ -d /opt/likewise ]; then
            /bin/rm -rf /opt/likewise
        fi
    fi
    return 0
}

install_darwin()
{
    uninstall_darwin

    for pkg in $INSTALL_BASE_PACKAGES ; do
        file=`echo ${PKGDIR}/${pkg}-[0-9]*.dmg`
        hdiutil attach "${file}"
        exit_on_error $? "Failed to attach ${file}"
        name=`basename "${file}" | sed -e 's/^\(.*\)\.dmg$/\1/'`
        exit_on_error $? "Failed to get package name from ${file}"
        installer -pkg /Volumes/${name}/${name}.mpkg -target /
        exit_on_error $? "Failed to install ${name} package"
        hdiutil detach /Volumes/${name}
        exit_on_error $? "Failed to detach /Volumes/${name}"
    done
    return 0
}

do_install()
{
    log_info "Installing packages"

    if [ "$PKGTYPE" = 'darwin' ]; then
        install_darwin
        return $?
    fi

    # Install upgrade helper packages.
    # First try to uninstall any previous upgrade package.
    # Then, check for the file and add to the install list.
    # Finally, install the packages in the list.
    pkgList=""
    for pkg in $INSTALL_UPGRADE_PACKAGE
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
        package_uninstall $pkgList
        err=$?
        if [ $err -ne 0 ]; then
            log_info "Error uninstalling $pkgList"
            exit 1
        fi
    fi

    pkgList=""
    for pkg in $INSTALL_UPGRADE_PACKAGE
    do
        pkgName=`package_file_exists $pkg`
        if [ $? -eq 0 ]; then
            if [ -n "${pkgList}" ]; then
                pkgList="${pkgList} $pkgName"
            else
                pkgList="$pkgName"
            fi
        fi
    done

    if [ -n "$pkgList" ]; then
        package_install "$pkgList"
        err=$?
        if [ $err -ne 0 ]; then
            log_info "Error installing $pkgList -- please check /tmp/LikewiseOpen.log for a full log."
            exit 1
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

    # Install all required packages.
    pkgList=""
    for pkg in $INSTALL_BASE_PACKAGES
    do
        pkgName=`package_file_exists $pkg`
        if [ $? -eq 0 ]; then
            if [ -n "${pkgList}" ]; then
                pkgList="${pkgList} $pkgName"
            else
                pkgList="$pkgName"
            fi
        else
            log_info "Missing file for package $pkg"
            exit 1
        fi
    done

    if [ -n "$pkgList" ]; then
        package_install "$pkgList"
        err=$?
        if [ $err -ne 0 ]; then
            log_info "Error installing $pkgList -- please check /tmp/LikewiseOpen.log for a full log."
            exit 1
        fi
    fi

    # Install all optional packages.
    for pkg in $INSTALL_OPTIONAL_PACKAGES
    do
        pkgName=`package_file_exists $pkg`
        if [ $? -eq 0 ]; then
            package_install "$pkgName"
            err=$?
            if [ $err -ne 0 ]; then
                if [ $err -eq $ERR_PACKAGE_COULD_NOT_INSTALL ]; then
                    log_info "Couldn't install optional package $pkgName"
                fi
            fi
        fi
    done

    mkdir -p /var/lib/likewise/uninstall
    echo "PREFIX=\"$PREFIX\"" > /var/lib/likewise/uninstall/MANIFEST
    echo "PKGTYPE=\"$PKGTYPE\"" >> /var/lib/likewise/uninstall/MANIFEST
    echo "INSTALL_UPGRADE_PACKAGE=\"$INSTALL_UPGRADE_PACKAGE\"" >> /var/lib/likewise/uninstall/MANIFEST
    echo "INSTALL_BASE_PACKAGES=\"$INSTALL_BASE_PACKAGES\"" >> /var/lib/likewise/uninstall/MANIFEST
    echo "INSTALL_OPTIONAL_PACKAGES=\"$INSTALL_OPTIONAL_PACKAGES\"" >> /var/lib/likewise/uninstall/MANIFEST
    if [ -f "${DIRNAME}/response" ]; then
        cp "${DIRNAME}/response" /var/lib/likewise/uninstall/response
    fi

    log_info "Installing Packages was successful"
}

get_ad_provider_path()
{
    ${PREFIX}/bin/lwregshell list_values '[HKEY_THIS_MACHINE\Services\lsass\Parameters\Providers\ActiveDirectory]' | grep '"Path"' | awk '{ print $3; }' | sed 's/"//g'
}

set_ad_provider_path()
{
     ${PREFIX}/bin/lwregshell set_value '[HKEY_THIS_MACHINE\Services\lsass\Parameters\Providers\ActiveDirectory]' 'Path' "$1"
}

# We are upgrading and need to be careful about what value may be present.
# This may be overly meticulous, but I want to make sure then have the path
# to the open provider.
switch_to_open_provider()
{
    # Test registry for Open 6.0 Active Directory provider path.
    if [ -z `get_ad_provider_path | grep liblsass_auth_provider_ad_open` ]; then
        new_value=""
        # Test registry for Enterprise 6.0 Active Directory provider path.
        if [ -z `get_ad_provider_path | grep liblsass_auth_provider_ad_enterprise` ]; then
            # Still has  5.4 value -- which is expected and good. Transform to
            # 6.0 open provider path.
            new_value=`get_ad_provider_path | sed 's/liblsass_auth_provider_ad/liblsass_auth_provider_ad_open/'`
        else
            # Has Enterprise 6.0 path (odd). Transform to Open 6.0 path.
            new_value=`get_ad_provider_path | sed 's/liblsass_auth_provider_ad_enterprise/liblsass_auth_provider_ad_open'`
        fi
        set_ad_provider_path "$new_value"
    fi
}

do_postinstall_messages()
{
    domainjoin_gui=`get_prefix_dir`/bin/domainjoin-gui
    run_join_gui=true
    guimsg=""

    if [ "$1" != 'interactive' ]; then
        run_join_gui=false
    fi

    if [ -x "$domainjoin_gui" ]; then
        guimsg="domainjoin-gui or "
    else
        run_join_gui=false
    fi

    if $OPT_DONT_JOIN
    then
        run_join_gui=false
    fi

    if [ -n "${UPGRADING}" ]; then
        log_info ""
        log_info "Likewise Open has been successfully upgraded."
        log_info ""

        command="`get_prefix_dir`/bin/lw-get-current-domain"
        domain=`$command 2>/dev/null`
        if [ $? -eq 0 ]; then
            domain=`echo $domain | sed -e 's/^Current Domain = //'`
            log_info "This computer is joined to $domain"
            log_info ""
        fi

        log_info "The nsswitch file has been modified."
        log_info "Please reboot so that all processes pick up the new copy."
        log_info ""
    else
        command="`get_prefix_dir`/bin/lw-get-current-domain"
        domain=`$command 2>/dev/null`
        if [ $? -eq 0 ]; then
            domain=`echo $domain | sed -e 's/^Current Domain = //'`
            log_info "This computer is joined to $domain"
            log_info ""
        else
            log_info ""
            log_info "As root, run ${guimsg}domainjoin-cli to join a domain so you can log on"
            log_info "with Active Directory credentials. Example:"
            log_info "domainjoin-cli join likewisedemo.com ADadminAccount"
            log_info ""

            if $run_join_gui
            then
                $domainjoin_gui >/dev/null 2>&1 &
            fi
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
    log_info "Uninstall started"

    if [ "$PKGTYPE" = "darwin" ]; then
        uninstall_darwin
        return $?
    fi

    pkgList=""
    for pkg in $INSTALL_UPGRADE_PACKAGE $INSTALL_OPTIONAL_PACKAGES $INSTALL_BASE_PACKAGES;
    do
        pkgName=`is_package_installed $pkg`
        if [ $? -eq 0 ]; then
            pkgList="$pkgList $pkgName"
        fi
    done
    package_uninstall $pkgList

    scrub_prefix
}

do_purge()
{
    log_info "Purge uninstall started"
    log_info ""

    domainjoin_cli=`get_prefix_dir`/bin/domainjoin-cli
    if [ -x "$domainjoin_cli" ]; then
        $domainjoin_cli leave > /dev/null 2>&1
    fi

    pkgList=""
    for pkg in $INSTALL_UPGRADE_PACKAGE $INSTALL_OPTIONAL_PACKAGES $INSTALL_BASE_PACKAGES;
    do
        pkgName=`is_package_installed $pkg`
        if [ $? -eq 0 ]; then
            pkgList="$pkgList $pkgName"
        fi
    done
    package_uninstall $pkgList

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

    prompt_yes_no "Would you like to install now?"
    if [ "x$answer" != "xyes" ]; then
        do_info
    else
        if [ -z "${OPT_COMPAT}" ]; then
            if [ -n "${HAVE_COMPAT}" ]; then
                prompt_yes_no "Would you like to install 32-bit compatibility libraries?" 1
                update_packages_list "${answer}"
            fi
        fi
        do_install
    fi
    exit 0
}

# must check before shift to avoid shift error on some sh versions.
check_arg_present()
{
    if [ -z "$1" ]; then
        if [ -n "$2" ]; then
            echo "Missing argument for $2"
        fi
        usage
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
    echo "    --compat         install 32-bit compatibility libraries (default: auto)"
    echo "    --nocompat       do not install 32-bit compatibility libraries (default: auto)"
    echo "    --devel          install development packages"
    #echo "    --type <pkgType> type of package to install"
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
    exit 1
}

main_install()
{
    OPT_DEVEL=false
    OPT_COMPAT=""

    ECHO_DIRNAME=""
    PKGTYPE=""

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
            --compat)
                if [ -n "${OPT_COMPAT}" ]; then
                    if [ "${OPT_COMPAT}" != "yes" ]; then
                        echo "Cannot use $1 with --nocompat"
                        usage
                    fi
                fi
                OPT_COMPAT=yes
                shift 1
                ;;
            --nocompat)
                if [ -n "${OPT_COMPAT}" ]; then
                    if [ "${OPT_COMPAT}" != "no" ]; then
                        echo "Cannot use $1 with --compat"
                        usage
                    fi
                fi
                OPT_COMPAT=no
                shift 1
                ;;
            --devel)
                OPT_DEVEL=true
                shift 1
                ;;
            --type|-t)
                if [ -n "${PKGTYPE}" ]; then
                    echo "Only one --type option allowed"
                    usage
                fi
                check_arg_present "$2" "$1"
                PKGTYPE="$2"
                shift 2
                ;;
            --ubuntu)
                do_ubuntu $2
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
            do_postinstall_messages
            ;;
        *)
            usage
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
    exit 1

}

main_uninstall()
{
    setup_os_vars

    if [ -f /var/lib/likewise/uninstall/MANIFEST ]; then
        . /var/lib/likewise/uninstall/MANFIEST
    else
        echo "The file /var/lib/likewise/uninstall/MANIFEST cannot be found and"
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

    if [ "$BASENAME" = "uninstall.sh" ]; then
        main_uninstall $@
    else
        main_install $@
    fi

    return 0
}

main "$@"
exit $?
