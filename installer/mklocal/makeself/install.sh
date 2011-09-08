#!/bin/sh
# ex: set tabstop=4 expandtab shiftwidth=4:

#
# Copyright (c) PowerBroker.  All rights reserved.
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

    # FIXME: Use different check for OS support (manifest value?)
    PKGDIR_RELATIVE="packages"
    PKGDIR="${DIRNAME}/${PKGDIR_RELATIVE}"
    if [ ! -d "${PKGDIR}" ]; then
        exit_on_error 1 "The installer does not support this OS (${OS_TYPE}) and architecture (${OS_ARCH})."
    fi

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

    if [ -x "/usr/sbin/selinuxenabled" -a -x "/usr/sbin/getenforce" ]; then
        if /usr/sbin/selinuxenabled >/dev/null 2>&1; then
            if /usr/sbin/getenforce 2>&1 | grep -v 'Permissive' >/dev/null 2>&1; then
                if [ -f /etc/selinux/config ]; then
                    exit_on_error 1 "SELinux found to be present, enabled, and enforcing.
SELinux must be disabled or set to permissive mode by editing the file
/etc/selinux/config and rebooting.
For instructions on how to edit the file to disable SELinux, see the SELinux man page."
                else
                    exit_on_error 1 "SELinux found to be present, enabled, and enforcing.
SELinux must be disabled or set to permissive mode.
Check your system's documentation for details."
                fi
            fi
        fi
    fi

}

package_file_exists_aix_bff()
{
    pkgFile=${PKGDIR}/$1.*.bff
    if [ -f $pkgFile ]; then
        echo "I:$1"
        return 0
    fi
    return $ERR_PACKAGE_FILE_NOT_FOUND
}

is_package_installed_aix_bff()
{
    lslpp -L "$1.*" > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo $1
        return 0
    fi
    return $ERR_PACKAGE_NOT_INSTALLED
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
    for pkg in $@; do
        eval "swremove -x mount_all_filesystems=false $@"
    done
    return 0
    return $ERR_PACKAGE_COULD_NOT_UNINSTALL
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
    eval "dpkg --remove $@"
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
    pkginfo -l | grep "VSTOCK:.*$1" > /dev/null 2>&1
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
        if [ $err -eq 1 ]; then
            return $ERR_PACKAGE_COULD_NOT_INSTALL
        fi
    done
    return 0
}

package_uninstall_solaris()
{
    for candidate in `pkginfo | awk '{print $2}' | grep '^LIKE' | sort -r`; do
        mpkg=`pkginfo -l $candidate | grep VSTOCK: | awk '{print $2;}'`
        for pkg in $@; do
            if [ "$mpkg" = "$pkg" ]; then
                pkgrm -a "${DIRNAME}/response" -n "$candidate"
                err=$?
                if [ $err -eq 1 ]; then
                    return $ERR_PACKAGE_COULD_NOT_UNINSTALL
                fi
            fi
        done
    done

    return 0
}

remove_extra_files()
{
    for file in /opt/pbis /etc/pbis /var/log/pbis /var/lib/pbis /var/cache/pbis ; do
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

get_prefix_dir()
{
    echo "${PREFIX}"
}

uninstall_darwin()
{
    # No easy way to uninstall individual packages on Mac OS X
    if [ -x /opt/pbis/bin/lwi-uninstall.sh ]; then
       /opt/pbis/bin/lwi-uninstall.sh

        if [ -d /opt/pbis ]; then
            /bin/rm -rf /opt/pbis
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

    # Install upgrade helper package.
    if [ -n "$INSTALL_UPGRADE_PACKAGE" ]; then
        pkgName=`is_package_installed $INSTALL_UPGRADE_PACKAGE`
        if [ $? -eq 0 ]; then
            package_uninstall $pkgName
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

    mkdir -p /var/lib/pbis/uninstall
    echo "PREFIX=\"$PREFIX\"" > /var/lib/pbis/uninstall/MANIFEST
    echo "PKGTYPE=\"$PKGTYPE\"" >> /var/lib/pbis/uninstall/MANIFEST
    echo "INSTALL_UPGRADE_PACKAGE=\"$INSTALL_UPGRADE_PACKAGE\"" >> /var/lib/pbis/uninstall/MANIFEST
    echo "INSTALL_BASE_PACKAGE=\"$INSTALL_BASE_PACKAGE\"" >> /var/lib/pbis/uninstall/MANIFEST
    echo "INSTALL_GUI_PACKAGE=\"$INSTALL_GUI_PACKAGE\"" >> /var/lib/pbis/uninstall/MANIFEST
    if [ -f "${DIRNAME}/response" ]; then
        cp "${DIRNAME}/response" /var/lib/pbis/uninstall/response
    fi

    log_info "Installing Packages was successful"
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
        log_info "PowerBroker Identity Services Open has been successfully upgraded."
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
            log_info "domainjoin-cli join YourDomain.com ADadminAccount"
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
    for pkg in $INSTALL_UPGRADE_PACKAGE $INSTALL_GUI_PACKAGE $INSTALL_BASE_PACKAGE;
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
    for pkg in $INSTALL_UPGRADE_PACKAGE $INSTALL_GUI_PACKAGE $INSTALL_BASE_PACKAGE;
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
    echo "    --devel          install development packages"
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
            --devel)
                OPT_DEVEL=true
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

    if [ -f /var/lib/pbis/uninstall/MANIFEST ]; then
        . /var/lib/pbis/uninstall/MANFIEST
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
