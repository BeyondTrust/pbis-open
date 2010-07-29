#!/bin/sh
# ex: set tabstop=4 expandtab shiftwidth=4:

#
# Copyright Likewise, 2006-2007.  All rights reserved.
#

log_info()
{
    echo "$@" 1>&2
}

setup_os_vars()
{
    OS_TYPE=`uname`
    exit_on_error $? "Could not determine OS type"
    OS_ARCH_COMMAND="uname -p"
    INIT_SCRIPT_PREFIX_DIR="/etc/init.d"

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

    ## initial package list variables

    ## Fill in packag lists from product manifest
    . `dirname $0`/MANIFEST

    case "${OS_TYPE}" in
        AIX)
            OS_TYPE='aix'
            INIT_SCRIPT_PREFIX_DIR="/etc/rc.d/init.d"
            ;;
        Linux)
            OS_TYPE='linux'
            # Use -m because it works on old Linux distros
            OS_ARCH_COMMAND="uname -m"
            ;;
        FreeBSD|"Isilon OneFS")
            OS_TYPE='freebsd'
            INIT_SCRIPT_PREFIX_DIR="/etc/rc.d"
            ;;
        SunOS)
            OS_TYPE='solaris'
            ;;
        Darwin)
            OS_TYPE='darwin'
            ;;
        HP-UX)
            OS_TYPE='hpux'
            OS_ARCH_COMMAND="getconf CPU_VERSION"
            INIT_SCRIPT_PREFIX_DIR="/sbin/init.d"
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

needle_in_list()
{
    _needle=$1
    shift
    _haystack=$@

    for _hay in $_haystack; do
        if [ "$_needle" = "$_hay" ]; then
            echo $_needle
            return 0
        fi
    done
    echo ""
    return 1
}

reverse_list()
{
    result=""
    for item in $@ ; do
        result="${item} ${result}"
    done
    echo "${result}"
}

find_pkg_type()
{
    if test "${OS_TYPE}" = "freebsd"
    then
        echo "freebsd"
    else
        for _find_pkg_type_file in "${PKGDIR}"/* ; do
            if [ -f "${_find_pkg_type_file}" ]; then
                _find_pkg_type_ext=`echo "${_find_pkg_type_file}" | sed -e 's/^.*\.\([^.]*\)$/\1/'`
                exit_on_error $? "Could not get extension for ${_find_pkg_type_file}"
                echo "${_find_pkg_type_ext}"
                return 0
            fi
        done
        exit_on_error 1 "Could not find ${PKGDIR}/*"
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

    if [ `"$id" -u` != 0 ]; then
        log_info "ERROR: Root privileges are required to install this software. Try running this installer with su or sudo."
        exit 1
    fi

    log_info "Checking setup environment..."
    setup_os_vars

    PKGDIR_RELATIVE="packages/${OS_TYPE}/${OS_ARCH}"
    PKGDIR="${DIRNAME}/${PKGDIR_RELATIVE}"
    if [ ! -d "${PKGDIR}" ]; then
        exit_on_error 1 "The installer does not support this OS (${OS_TYPE}) and architecture (${OS_ARCH})."
    fi

    HAVE_COMPAT=""
    if [ -d "${PKGDIR}/compat" ]; then
        HAVE_COMPAT=1
    fi

    if [ -z "${PKGTYPE}" ]; then
        PKGTYPE=`find_pkg_type`
        exit_on_error $? "Could not determine package type"
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

get_rpm_version()
{
    _get_rpm_version=`rpm -q --queryformat "%{VERSION}\n" $1`
    if [ $? -ne 0 ]; then
        return 1
    fi
    # Only return the first entry in case we somehow have multiple.
    echo "${_get_rpm_version}" | head -1
    return 0
}

check_rpm_installed()
{
    rpm -q $1 > /dev/null 2>&1
}

check_bff_installed()
{
    lslpp -L $1 > /dev/null 2>&1
}

check_deb_installed()
{
    _status="`dpkg -s "$1" 2>/dev/null`"
    if [ $? -ne 0 ]
    then
        return 1
    fi

    if echo "$_status" | grep 'not-installed' >/dev/null 2>&1
    then
        return 1
    fi

    return 0
}

check_pkg_installed()
{
    pkginfo $1 > /dev/null 2>&1
}

check_depot_installed()
{
    swlist -l subproduct | grep "$1" >/dev/null 2>&1
}

check_freebsd_installed()
{
    pkg_info "$1-*" >/dev/null 2>&1
}

stop_daemons()
{
    for daemon in `reverse_list ${DAEMONS}`; do
        stop_daemon $daemon
    done
}

stop_obsolete_daemons()
{
    for daemon in ${OBSOLETE_DAEMONS}
    do
        stop_daemon $daemon
    done
    if type svccfg >/dev/null 2>&1; then
        for daemon in ${OBSOLETE_DAEMONS}; do
            if svccfg select $daemon 2>/dev/null; then
                svccfg delete $daemon
            fi
        done
    fi
}

stop_daemons_on_reboot()
{
    for daemon in ${DAEMONS} ${OBSOLETE_DAEMONS}; do
        # Ubuntu, Debian, Solaris, and Redhat
        rm -f /etc/rc?.d/S??${daemon} /etc/rc?.d/K??${daemon}

        # AIX, and Redhat
        rm -f /etc/rc.d/rc?.d/S??${daemon} /etc/rc.d/rc?.d/K??${daemon}

        # HP-UX, old likewise install
        rm -f /sbin/rc?.d/S??${daemon} /sbin/rc?.d/K??${daemon}
        # HP-UX, new likewise install
        rm -f /sbin/rc?.d/S???${daemon} /sbin/rc?.d/K???${daemon}

        # Solaris 10 and newer
        if type svccfg >/dev/null 2>&1 && svccfg select $daemon 2>/dev/null; then
            svcadm disable $daemon
            svccfg delete $daemon
        fi

        # Simply deleting the init scripts in /etc/rc.d (already happened
        # through package uninstall) is all that is necessary for FreeBSD.

        # OS X
        if [ -x /bin/launchctl ] ; then
            for file in /Library/LaunchDaemons/*${daemon}.plist /System/Library/LaunchDaemons/*${daemon}.plist; do
                if [ -f $file ]; then
                    /bin/launchctl unload $file
                    rm -f $file
                fi
            done
        fi
    done
}

index_of()
{
    awk 'END { print index(myvalue,i) }' myvalue="$1", i="$2" /dev/null
}

# Changes ownership of files belonging to $1 (bff file) after installation
# TODO: Why is this necessary?
bff_chown()
{
    MARKER="/inst_root/"
    for file in `restore -Tq -f $1 2>/dev/null | tr '\n' ' '` ; do
        INDEX=`index_of "$file" "${MARKER}"`
        exit_on_error $? "Failed to run awk"
        # INDEX is 1-based index of first char in MARKER
        if [ $INDEX -ne 0 ]; then
            # Note that expr substr below uses a 1-based index
            START_INDEX=`expr $INDEX + length "$MARKER" - 1`
            exit_on_error $? "Failed to run expr"
            REM_LEN=`expr length "$file" - $START_INDEX + 1`
            exit_on_error $? "Failed to run expr"
            # Only worry about stuft that is beyond /
            if [ $REM_LEN -gt 1 ]; then
                inst_file=`expr substr "$file" $START_INDEX $REM_LEN`
                exit_on_error $? "Failed to run expr"
                if [ -f "${inst_file}" ]; then
                    chown root:system "${inst_file}"
                fi
            fi
        fi
    done
}

install_bffs()
{
    geninstall -I"aX" -d "${PKGDIR}" all
    exit_on_error $? "Failed to install packages"

    for file in "${PKGDIR}"/*.bff ; do
        bff_chown $file
    done

    return 0
}

install_rpms()
{
    case "${OS_TYPE}" in
        aix)
            PACKAGE_SPEC=
            for file in "${PKGDIR}"/*.rpm ; do
                _basename=`basename "${file}"`
                PACKAGE_SPEC="${PACKAGE_SPEC} R:`basename ${_basename}`"
            done
            geninstall -d "${PKGDIR}" -F ${PACKAGE_SPEC}
            exit_on_error $? "Failed to install packages"
            ;;
        linux)

             # If not installing the compatlibs, remove any already installed.
            if [ -z "${IS_COMPAT}" ]; then
                uninstall_rpms "`get_installed_compat_rpms`"
            fi

            _compatlibs=""
            if [ -n "${IS_COMPAT}" ]; then
                _compatlibs="${PKGDIR}"/compat/*.rpm
            fi

            _rpms=""
            for _pkg in ${PACKAGES}
            do
                if [ -z "`needle_in_list $_pkg $OPTIONAL_PACKAGES`" ]; then
                    _rpms="$_rpms ${PKGDIR}/${_pkg}-[0-9]*.rpm"
                fi
            done

            rpm ${RPM_INSTALL_OPTIONS} ${_rpms} ${_compatlibs}
            exit_on_error $? "Failed to install packages"

            for _pkg in "$OPTIONAL_PACKAGES"
            do
                if [ -f "${PKGDIR}"/"${_pkg}"-[0-9]*.rpm ]; then
                    rpm ${RPM_INSTALL_OPTIONS} "${PKGDIR}"/"${_pkg}"-[0-9]*.rpm  2>&1
                    if [ $? -ne 0 ]; then
                        echo "Optional rpm ${_pkg} failed to install -- ok"
                    fi
                fi
            done

            ;;
        *)
            exit_on_error 1 "Unexpected OS \"${OS_TYPE}\" for installing RPMs"
            ;;
    esac
    return 0
}

install_debs()
{
    _debs=""

    for _pkg in ${PACKAGES}
    do
        _debs="$_debs ${PKGDIR}/${_pkg}_*.deb"

        if $OPT_DEVEL
        then
            if test -f ${PKGDIR}/${_pkg}-dev_*.deb
            then
                _debs="$_debs ${PKGDIR}/${_pkg}-dev_*.deb"
            fi
        fi
    done

    dpkg ${DPKG_INSTALL_OPTIONS} ${_debs}
    exit_on_error $? "Failed to install packages"
    return 0
}

install_dmgs()
{
    # On Mac OS X, we need to uninstall first since installing on top of an old
    # install results in errors.

    dispatch_pkgtype uninstall `reverse_list ${PACKAGES} ${PACKAGES_COMPAT}`
    for pkg in $@ ; do
        file=`echo ${PKGDIR}/${pkg}-*.dmg`
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

install_pkgs()
{
    for pkg in $@ ; do
        pkgadd -a "${DIRNAME}/response" -d "${PKGDIR}/${pkg}"-*.pkg all
        exit_on_error $? "Failed to install package ${pkg}"
    done
    return 0
}

install_depots()
{
    # Need to get the absolute path of the depot file

    for pkg in $@ ; do
        swinstall -x match_target=true -x mount_all_filesystems=false -s "${PKGDIR}/${pkg}"-*.depot \*
        exit_on_error $? "Failed to install package ${pkg}"
    done
    return 0
}

install_freebsds()
{
    for pkg in $@
    do
        echo "Installing ${pkg}"
        if [ -z "`needle_in_list $pkg $OPTIONAL_PACKAGES`" ]; then
            pkg_add ${PKGDIR}/${pkg}-[0-9]*.tbz
            exit_on_error $? "Failed to install package ${pkg}"
        else
            pkg_add ${PKGDIR}/${pkg}-[0-9]*.tbz
        fi
    done
    return 0
}

uninstall_depots()
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

uninstall_bffs()
{
    for pkg in $@ ; do
        check_bff_installed ${pkg}
        if [ $? -eq 0 ]; then
            geninstall -u ${pkg}
            exit_on_error $? "Failed to uninstall packages"
        fi
    done
    return 0
}

get_installed_compat_rpms()
{
    rpm -qa|grep likewise-|grep 32bit\.
    return 0
}

uninstall_rpms()
{
    _pkgs=""
    for pkg in $@ ; do
        check_rpm_installed ${pkg}
        if [ $? -eq 0 ]; then
            _pkgs="${_pkgs} ${pkg}"
        fi
    done
    if [ -n "${_pkgs}" ]; then
        rpm -e ${_pkgs}
        exit_on_error $? "Failed to uninstall packages"
    fi
    return 0
}

uninstall_debs()
{
    _pkgs=""
    for pkg in $@ ; do
        if check_deb_installed ${pkg}
        then
            _pkgs="${_pkgs} ${pkg}"
        fi

        if check_deb_installed "${pkg}-dev"
        then
            _pkgs="${_pkgs} ${pkg}-dev"
        fi
    done
    if [ -n "${_pkgs}" ]; then
        dpkg --purge ${_pkgs}
        exit_on_error $? "Failed to uninstall packages"
    fi
    return 0
}

uninstall_pkgs()
{
    for pkg in $@ ; do
        check_pkg_installed ${pkg}
        if [ $? -eq 0 ]; then
            # TODO: Is the response file good for uninstall?
            pkgrm -a "${DIRNAME}/response" -n ${pkg}
            exit_on_error $? "Failed to uninstall package ${pkg}"
        fi
    done
    return 0
}

uninstall_dmgs()
{
    #
    # No easy way to uninstall individual packages on Mac OS X
    #
    if [ -x /opt/likewise/bin/lwi-uninstall.sh ]; then
       /opt/likewise/bin/lwi-uninstall.sh
    fi
    if [ -d /opt/likewise ]; then
       /bin/rm -rf /opt/likewise
    fi
    return 0
}

uninstall_freebsds()
{
    for pkg in $@
    do
        if check_freebsd_installed "${pkg}"
        then
            echo "Removing ${pkg}"
            pkg_delete "${pkg}-*"
            exit_on_error $? "Failed to uninstall package ${pkg}"
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

rpm_pkg_version()
{
    rpm -q "$1" --queryformat "%{VERSION}"
}

deb_pkg_version()
{
    dpkg-query -s "$1" | grep '^Version:' | awk '{ print $2; }' | sed 's/-.*$//'
}

convert_version()
{
    awk 'BEGIN { FS = "."; } { printf $1*100.$2; }'
}

version_less_than()
{
    first="`echo $1 | convert_version`"
    second="`echo $2 | convert_version`"
    test $first -lt $second
}

preinstall_rpms()
{
    pkg=""
    obsolete=""
    for pkg in ${OBSOLETE_PACKAGES}
    do
        if check_rpm_installed "${pkg}"
        then
            echo "Removing obsolete package: `rpm -q ${pkg}`"
            obsolete="$obsolete $pkg"
        fi
    done
    uninstall_rpms $obsolete
}

preinstall_debs()
{
    pkg=""
    obsolete=""
    for pkg in ${OBSOLETE_PACKAGES}
    do
        if check_deb_installed "${pkg}"
        then
            echo "Removing obsolete package: ${pkg}"
            obsolete="$obsolete $pkg"
        fi
    done
    uninstall_debs $obsolete
}

preinstall_pkgs()
{
    FIXUP_DIRS="/etc/samba /var/lib /var/log/lwidentity /var/cache/likewise"

    for _dir in ${FIXUP_DIRS}; do
        [ -d "$_dir" ] && chown -R root:other "$_dir"
    done
    echo "Uninstalling previous install"
    uninstall_pkgs `reverse_list "$@"`
    
    echo "Creating base directory"
    mkdir -p "`get_prefix_dir`"
}

preinstall_depots()
{
    echo "Uninstalling previous install"
    uninstall_depots ${OBSOLETE_PACKAGES} `reverse_list "$@"`
}

preinstall_bffs()
{
    echo "Uninstalling previous install"
    uninstall_bffs ${OBSOLETE_PACKAGES} `reverse_list "$@"`
}

preinstall_freebsds()
{
    echo "Uninstalling previous install"
    uninstall_freebsds ${OBSOLETE_PACKAGES} `reverse_list "$@"`
}

check_daemon_running()
{
    for script in $@ ; do
        if [ -f $INIT_SCRIPT_PREFIX_DIR/${script} ]; then
            $INIT_SCRIPT_PREFIX_DIR/${script} status > /dev/null 2>&1
            rc=$?
            if [ $rc -eq 0 ]; then
                return 0
            fi
        fi
    done
    return 1
}

dispatch_pkgtype()
{
    type "$1_${PKGTYPE}s" > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        exit_on_error $? "Invalid package type: ${PKGTYPE}"
    fi
    __dispatch_pkgtype="$1"
    shift
    ${__dispatch_pkgtype}_${PKGTYPE}s "$@"
}

start_daemon()
{
    if [ -x $INIT_SCRIPT_PREFIX_DIR/$1 ]; then
        $INIT_SCRIPT_PREFIX_DIR/$1 status > /dev/null 2>&1
        if [ $? -ne 0 ]; then
            if [ -n "$2" ]; then
                log_info "Starting $1 daemon"
            fi
            if type svccfg >/dev/null 2>&1; then
                if svccfg select TEMP/network/$1 2>/dev/null; then
                    svccfg delete TEMP/network/$1
                fi
                svccfg import /etc/likewise/svcs-solaris/$1.xml
            fi
            $INIT_SCRIPT_PREFIX_DIR/$1 start
        fi
    fi
}

kill_process()
{
    pkill -KILL -x $1 > /dev/null 2> /dev/null
}

kill_likewise_daemons()
{
    kill_process srvsvcd
    kill_process lsassd
    kill_process lwiod
    kill_process netlogond
    kill_process eventlogd
    kill_process dcerpcd
    kill_process netlogond
    kill_process lwsmd
    kill_process lwregd
}

stop_daemon()
{
    if [ -x $INIT_SCRIPT_PREFIX_DIR/$1 ]; then
        $INIT_SCRIPT_PREFIX_DIR/$1 status > /dev/null 2>&1
        if [ $? -eq 0 ]; then
            if [ -n "$2" ]; then
                log_info "Stopping $1 daemon"
            fi
            $INIT_SCRIPT_PREFIX_DIR/$1 stop
        fi
    fi
}

reload_daemon()
{
    if [ -x $INIT_SCRIPT_PREFIX_DIR/$1 ]; then
        $INIT_SCRIPT_PREFIX_DIR/$1 reload
    fi
}

save_daemon()
{
    if check_daemon_running "$1"
    then
        eval "${1}_RUNNING=1"
        stop_daemon "${1}"
    fi
}

restore_daemon()
{
    eval "running=\$${1}_RUNNING"
    if [ -n "${running}" ]
    then
        start_daemon "${1}"
    fi
}

save_daemons()
{
    daemon=""
    for daemon in `reverse_list ${DAEMONS}`
    do
        save_daemon "${daemon}"
    done
}

restore_daemons()
{
    daemon=""
    for daemon in ${DAEMONS}
    do
        restore_daemon "${daemon}"
    done
}

autostart_daemons()
{
    daemon=""
    for daemon in ${AUTOSTART}
    do
        start_daemon "${daemon}"
    done
}

start_daemon_lwsmd()
{
    LWSMD=`get_prefix_dir`/sbin/lwsmd

    $LWSMD --start-as-daemon
}

determine_upgrade_type()
{
    VERSIONFILE=`get_prefix_dir`/data/VERSION
    if [ -f $VERSIONFILE ]; then
        UPGRADING=1
        if [ -n "`grep '^VERSION=5.0' $VERSIONFILE`" -o \
             -n "`grep '^VERSION=5.1' $VERSIONFILE`" -o \
             -n "`grep '^VERSION=5.2' $VERSIONFILE`" -o \
             -n "`grep '^VERSION=5.3' $VERSIONFILE`" ]; then
            UPGRADING_FROM_5_0123=1
            UPGRADEDIR5=/tmp/lw-upgrade
            mkdir -p "${UPGRADEDIR5}"
            log_info "Preserving 5.x (0 <= x <= 3) configuration in ${UPGRADEDIR5}."
        elif [ -n "`grep '^VERSION=5.4' $VERSIONFILE`" ]; then
            UPGRADING_FROM_5_4=1
            UPGRADEDIR5=/tmp/lw-upgrade
            mkdir -p "${UPGRADEDIR5}"
            log_info "Preserving 5.4 configuration in ${UPGRADEDIR5}."
        fi
    elif [ -x /usr/bin/dpkg ]; then
        if check_deb_installed likewise-open
        then
            if [ -f /usr/sbin/lsassd ]; then
                log_info "You have the likewise-open package from Ubuntu installed."
                log_info "Upgrading from the likewise-open package is not supported."
                exit 1
            fi
        fi
    fi
}

preserve_5_0123_configuration()
{
    if [ -n "${UPGRADING_FROM_5_0123}" ]; then
        if [ -f "/etc/likewise/eventlogd.conf" ]; then
            cp "/etc/likewise/eventlogd.conf" "${UPGRADEDIR5}"
        fi

        if [ -f "/etc/likewise/lsassd.conf" ]; then
            cp "/etc/likewise/lsassd.conf" "${UPGRADEDIR5}"
        fi

        if [ -f "/etc/likewise/netlogon.conf" ]; then
            cp "/etc/likewise/netlogon.conf" "${UPGRADEDIR5}"
        fi

        if [ -f "/var/lib/likewise/db/pstore.db" ]; then
            cp "/var/lib/likewise/db/pstore.db" "${UPGRADEDIR5}"
            chmod 700 "${UPGRADEDIR5}/pstore.db"
        fi
    fi
}

preserve_5_4_configuration()
{
    if [ -n "${UPGRADING_FROM_5_4}" ]; then
        for file in dcerpcd.reg eventlogd.reg lsassd.reg lwiod.reg lwreg.reg netlogond.reg pstore.reg srvsvcd.reg
        do
            if [ -f "/etc/likewise/${file}" ]; then
                cp "/etc/likewise/${file}" "${UPGRADEDIR5}"
            fi
        done
    fi
}

import_5_0123_file()
{
    CONVERT="`get_prefix_dir`/bin/conf2reg"
    REGIMPORT="`get_prefix_dir`/bin/lwregshell import"

    COMMAND=$1
    SOURCE=$2
    # DEST is not necessary for some commands.
    DEST=$3

    if [ -f $SOURCE ]; then
        $CONVERT $COMMAND $SOURCE $DEST > /dev/null 2>&1
        if [ $? -ne 0 ]; then
            log_info "Error converting ${SOURCE} to new format."
            return 1
        fi

        if [ -n "$DEST" -a -f "$DEST" ]; then
            $REGIMPORT $DEST > /dev/null 2>&1
            if [ $? -ne 0 ]; then
                log_info "Error importing ${DEST} into the registry."
                return 1
            fi
        fi
        return 0
    fi
    log_info "Unable to find ${SOURCE}."
    return 1
}

restore_5_0123_configuration()
{
    CONVERT="`get_prefix_dir`/bin/conf2reg"

    if [ -z "${UPGRADING_FROM_5_0123}" ]; then
        return 0;
    fi

    import_5_0123_file "--lsass" "${UPGRADEDIR5}/lsassd.conf" \
        "${UPGRADEDIR5}/lsassd.conf.reg"

    import_5_0123_file "--netlogon" "${UPGRADEDIR5}/netlogon.conf" \
        "${UPGRADEDIR5}/netlogon.conf.reg"

    import_5_0123_file "--eventlog" "${UPGRADEDIR5}/eventlogd.conf" \
        "${UPGRADEDIR5}/eventlogd.conf.reg"

    import_5_0123_file "--pstore-sqlite" "${UPGRADEDIR5}/pstore.db"
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

import_registry()
{
    REGIMPORT="`get_prefix_dir`/bin/lwregshell upgrade"

    for regfile in ${REGFILES}
    do
        FILEPATH="`get_prefix_dir`/share/config/${regfile}"
        if [ -f ${FILEPATH} ]
        then
            ${REGIMPORT} ${FILEPATH} > /dev/null 2>&1
            exitcode=$?

            if [ $exitcode -ne '0' ]
            then
                echo "There was an error importing ${FILEPATH} into the registry."
            fi
        fi
    done
}

get_prefix_dir()
{
    echo "${PREFIX}"
}

do_install_ubuntu_desktop_shortcut()
{
    # Check if user is not root and desktop shortcut exists
    if [ `id -u $SUDO_USER` -ne 0 -a -f $HOME/Desktop/Likewise\ Domain\ Join\ Tool.desktop ]; then
        # Add execute permission for local user
        sudo chmod 745 $HOME/Desktop/Likewise\ Domain\ Join\ Tool.desktop
        # Check whether gksudo is executable or not
        if [ -x /usr/bin/gksudo ]; then
            # Replace Exec= with Exec=/usr/bin/gksudo 
            sudo sed -i 's/Exec=/Exec=\/usr\/bin\/gksudo /' $HOME/Desktop/Likewise\ Domain\ Join\ Tool.desktop
        fi
    fi
    exit $?
}

do_ubuntu()
{
    if [ -f /etc/lsb-release ]; then
        grep -i ubuntu /etc/lsb-release >> /dev/null
        if [ $? = 0 ]; then
            case "$1" in
                install)
                    # do_install_ubuntu_desktop_shortcut
                    ;;
           esac
        fi
    fi
    exit $?
}

do_install()
{
    log_info "Install started"
    log_info ""

    # Determine if we are upgrading and what we are upgrading from.
    determine_upgrade_type

    # Stop obsolete daemons
    stop_obsolete_daemons

    # Save the daemon state
    save_daemons

    # Save 5.[0123] configuration files and pstore.
    preserve_5_0123_configuration

    # Save 5.4 registry files.
    preserve_5_4_configuration

    prefix_dir=`get_prefix_dir`
    if [ $? -eq 0 -a -d "$prefix_dir" ]; then
        #If the Bitrock installer was called with a restrictive umask, the
        #prefix directory could get created with too restrictive of
        #permissions. This will fix that.
        chmod 755 "$prefix_dir"
    fi

    dispatch_pkgtype preinstall ${PACKAGES}

    # Likewise Identity 5.3.0.7798 with purge leaves daemons running -- kill them
    kill_likewise_daemons

    # Do the install/upgrade
    dispatch_pkgtype install ${PACKAGES}

    do_postinstall

    log_info "Install complete"
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

do_postinstall()
{
    # Start service manager for registry import *** NOT USING SYSTEM SCRIPT
    start_daemon_lwsmd

    # Import configuration files and the old pstore into the registry
    restore_5_0123_configuration

    # Populate registry
    import_registry

    # Update AD provider path in 5.4 registry to 6.0 name.
    fix_5_4_configuration

    fix_old_registry

    # Stop service manager for registry import *** WILL RESTART NORMALLY
    stop_daemon lwsmd

    # Start service manager for normal usage
    start_daemon lwsmd

    # Tell service manager to re-read service list
    reload_daemon lwsmd

    # Restore the daemon state
    restore_daemons

    # Start any automatically-started daemons
    autostart_daemons

    # Restore configuration if still joined to a domain
    restore_configuration
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

restore_configuration()
{
    domainjoin_cli=`get_prefix_dir`/bin/domainjoin-cli
    get_current_domain=`get_prefix_dir`/bin/lw-get-current-domain

    # This starts all needed likewise services
    if [ -x "$domainjoin_cli" ]; then
        $domainjoin_cli query > /dev/null 2>&1
    fi

    if [ -x "$get_current_domain" ]; then
        $get_current_domain > /dev/null 2>&1
        if [ $? -eq 0 ]; then
            if [ -x "$domainjoin_cli" ]; then
                $domainjoin_cli configure --enable pam > /dev/null 2>&1
                $domainjoin_cli configure --enable nsswitch > /dev/null 2>&1
            fi
        fi
    fi
}

do_uninstall()
{
    log_info "Uninstall started"
    log_info ""

    domainjoin_cli=`get_prefix_dir`/bin/domainjoin-cli
    if [ -x "$domainjoin_cli" ]; then
        $domainjoin_cli configure --disable pam > /dev/null 2>&1
        $domainjoin_cli configure --disable nsswitch > /dev/null 2>&1
    fi

    stop_daemons

    dispatch_pkgtype uninstall `reverse_list ${PACKAGES} ${PACKAGES_COMPAT} ${OBSOLETE_PACKAGES}`

    scrub_prefix

    stop_daemons_on_reboot

    log_info "Uninstall complete"
}

do_purge()
{
    log_info "Purge uninstall started"
    log_info ""

    domainjoin_cli=`get_prefix_dir`/bin/domainjoin-cli
    if [ -x "$domainjoin_cli" ]; then
        $domainjoin_cli leave > /dev/null 2>&1
        $domainjoin_cli configure --disable pam > /dev/null 2>&1
        $domainjoin_cli configure --disable nsswitch > /dev/null 2>&1
    fi

    stop_daemons

    dispatch_pkgtype uninstall `reverse_list ${PACKAGES} ${PACKAGES_COMPAT}`

    remove_extra_files

    scrub_prefix

    stop_daemons_on_reboot

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
        exit 0
    else
        if [ -z "${OPT_COMPAT}" ]; then
            if [ -n "${HAVE_COMPAT}" ]; then
                prompt_yes_no "Would you like to install 32-bit compatibility libraries?" 1
                update_packages_list "${answer}"
            fi
        fi
        do_install
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
    echo "    --dont-join      do not run the domainjoin GUI tool after install completes (default: auto)"
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

main()
{
    OPT_DEVEL=false
    OPT_DONT_JOIN=false
    OPT_COMPAT=""
    DIRNAME=`dirname $0`
    if [ -z "${DIRNAME}" ]; then
        DIRNAME=.
    fi
    if echo "$DIRNAME" | grep -v '^/' >/dev/null 2>&1; then
        DIRNAME="`pwd`/$DIRNAME"
    fi

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
            --dont-join)
                OPT_DONT_JOIN=true
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
        postinstall)
            setup_os_vars
            do_postinstall
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
            do_postinstall_messages 'interactive'
            ;;
        *)
            usage
            ;;
    esac

    return 0
}

main "$@"
exit $?
