#!/bin/sh
# ex: set tabstop=4 expandtab shiftwidth=4:

#
# Copyright Likewise, 2006-2007.  All rights reserved.
#

# TODO - perhaps remove obsolete packages when upgrading for rpm and deb.

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

do_setup()
{
    umask 022

    if [ ! -d "${DIRNAME}/packages" ]; then
        log_info "ERROR: Directory ${DIRNAME}/packages is not present."
        log_info "       Perhaps you need to use the --dir option."
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
    dpkg -s $1 > /dev/null 2>&1
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
}

stop_daemons_on_reboot()
{
    for daemon in ${DAEMONS} ${OBSOLETE_DAEMONS}; do
        # Ubuntu, Debian, Solaris, and Redhat
        rm -f /etc/rc?.d/S??${daemon} /etc/rc?.d/K??${daemon}

        # AIX, and Redhat
        rm -f /etc/rc.d/rc?.d/S??${daemon} /etc/rc.d/rc?.d/K??${daemon}

        # HP-UX
        rm -f /sbin/rc?.d/S??${daemon} /sbin/rc?.d/K??${daemon}

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
                uninstall_rpms `reverse_list ${PACKAGES_COMPAT}`
            fi

            _compatlibs=""
            if [ -n "${IS_COMPAT}" ]; then
                _compatlibs="${PKGDIR}"/compat/*.rpm
            fi
            rpm ${RPM_INSTALL_OPTIONS} "${PKGDIR}"/*.rpm ${_compatlibs}
            exit_on_error $? "Failed to install packages"
            ;;
        *)
            exit_on_error 1 "Unexpected OS \"${OS_TYPE}\" for installing RPMs"
            ;;
    esac
    return 0
}

install_debs()
{
    dpkg ${DPKG_INSTALL_OPTIONS} "${PKGDIR}"/*.deb
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
        pkg_add ${PKGDIR}/${pkg}-*.tbz
        exit_on_error $? "Failed to install package ${pkg}"
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
        check_deb_installed ${pkg}
        if [ $? -eq 0 ]; then
            _pkgs="${_pkgs} ${pkg}"
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
            $INIT_SCRIPT_PREFIX_DIR/$1 start
        fi
    fi
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


import_registry()
{
    REGIMPORT="`get_prefix_dir`/bin/lwregshell upgrade"

    for regfile in ${REGFILES}
    do
        FILEPATH="/etc/likewise/${regfile}"
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

upgrade_pstore()
{
    PSTORE_PATH=/var/lib/likewise/db/pstore.db
    SQLITE3_PATH=`get_prefix_dir`/bin/sqlite3

    if [ -w $PSTORE_PATH -a -x $SQLITE3_PATH ]; then
         echo alter table machinepwd add column HostDnsDomain varchar\(256\) default NULL\; | ${SQLITE3_PATH} ${PSTORE_PATH} >/dev/null
    fi
}

upgrade_databases()
{
    upgrade_pstore
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

    # Stop obsolete daemons
    stop_obsolete_daemons

    # Save the daemon state
    save_daemons

    prefix_dir=`get_prefix_dir`
    if [ $? -eq 0 -a -d "$prefix_dir" ]; then
        #If the Bitrock installer was called with a restrictive umask, the
        #prefix directory could get created with too restrictive of
        #permissions. This will fix that.
        chmod 755 "$prefix_dir"
    fi

    dispatch_pkgtype preinstall ${PACKAGES}
    # Do the install/upgrade
    dispatch_pkgtype install ${PACKAGES}

    do_postinstall

    log_info "Install complete"
}

do_postinstall()
{
    # Upgrade database schemas
    upgrade_databases

    # Start service manager for registry import *** NOT USING SYSTEM SCRIPT
    start_daemon_lwsmd

    # Popuplate registry
    import_registry

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
    stop_daemons

    domainjoin_cli=`get_prefix_dir`/bin/domainjoin-cli
    if [ -x "$domainjoin_cli" ]; then
        $domainjoin_cli configure --disable pam > /dev/null 2>&1
        $domainjoin_cli configure --disable nsswitch > /dev/null 2>&1
    fi

    dispatch_pkgtype uninstall `reverse_list ${PACKAGES} ${PACKAGES_COMPAT}`

    scrub_prefix

    stop_daemons_on_reboot

    log_info "Uninstall complete"
}

do_purge()
{
    log_info "Purge uninstall started"
    log_info ""
    stop_daemons

    domainjoin_cli=`get_prefix_dir`/bin/domainjoin-cli
    if [ -x "$domainjoin_cli" ]; then
        $domainjoin_cli leave > /dev/null 2>&1
    fi

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
            ;;
        postinstall)
            setup_os_vars
            do_postinstall
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
            ;;
        *)
            usage
            ;;
    esac

    return 0
}

main "$@"
exit $?
