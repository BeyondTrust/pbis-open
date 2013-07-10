#!/bin/sh

# This script completely uninstalls PowerBroker Identity Services from the Mac

warn()
{
    echo "$@" 1>&2
}

exit_on_error()
{
    if [ $1 -ne 0 ]; then
        if [ -n "$2" ]; then
            warn "$2"
        else
            warn "Exiting on error $1"
        fi
        exit $1
    fi
}

gather_version_info()
{
    IS_10_4=
    IS_10_5=
    IS_10_6=
    IS_10_7=
    IS_10_8=
    IS_10_9=
    HAVE_LAUNCHCTL_V2=

    _get_os_version=`uname -r`
    case "${_get_os_version}" in
        8.*)
            IS_10_4=1
            echo "This is a Tiger (10.4) version of Mac OS X"
            ;;
        9.*)
            IS_10_5=1
            echo "This is a Leopard (10.5) version of Mac OS X"
            HAVE_LAUNCHCTL_V2=1
            ;;
        10.*)
            IS_10_6=1
            echo "This is a Snow Leopard (10.6) version of Mac OS X"
            HAVE_LAUNCHCTL_V2=1
            ;;
        11.*)
            IS_10_7=1
            echo "This is a Lion (10.7) version of Mac OS X"
            HAVE_LAUNCHCTL_V2=1
            ;;
        12.*)
            IS_10_8=1
            echo "This is a Mountain Lion (10.8) version of Mac OS X"
            HAVE_LAUNCHCTL_V2=1
            ;;
        13.*)
            IS_10_9=1
            echo "This is a Mavericks (10.9) version of Mac OS X"
            HAVE_LAUNCHCTL_V2=1
            ;;
        *)
            echo "Unsupported Mac OS X version (uname -r = '${_get_os_version}')."
            return 1
    esac
    return 0
}

pkg_list_pbis()
{
    if [ -d /Library/Receipts ]; then
        ls -1d /Library/Receipts/pbis-*.pkg 2>/dev/null | tr '\n' '\0' | xargs -0 basename
    fi

    if [ -d /var/db/receipts ]; then
        ls -1d /var/db/receipts/com.beyondtrust.pbis.*.bom 2>/dev/null | sed -e 's/com.beyondtrust.//g' | sed -e 's/.pkg.bom/.pkg/g' | tr '\n' '\0' | xargs -0 basename
    fi
}

pkg_uninstall_rm()
{
    if [ -n "$1" ]; then
        if [ -d "$1" ]; then
            $RUN rmdir "$1" 2>/dev/null
            if [ $? -eq 0 ]; then
                if [ -z "$RUN" ]; then
                    echo "Removed directory '$1'"
                fi
            fi
        else
            $RUN rm -fv "$1"
        fi
    fi
}

pkg_uninstall()
{
    # Mac xargs does not work properly in some cases with sh -c '...$1...' ARG
    # or with -IX to do direct replacement.  So, instead, we just want to
    # invoke a command directly.

    if [ -d "/Library/Receipts/$1" ]; then
        lsbom -pf "/Library/Receipts/$1/Contents/Archive.bom" | sed -e 's/^\.//' | sort -r | tr '\n' '\0' | xargs -0 -n1 $0 pkg_uninstall_rm "${RUN}"
        $RUN rm -rfv "/Library/Receipts/$1"
    elif [ -f "/var/db/receipts/com.beyondtrust.$1.bom" ]; then
        lsbom -pf "/var/db/receipts/com.beyondtrust.$1.bom" | sed -e 's/^\.//' | sort -r | tr '\n' '\0' | xargs -0 -n1 $0 pkg_uninstall_rm "${RUN}"
        $RUN rm -rfv "/var/db/receipts/com.beyondtrust.$1.bom"
        $RUN rm -rfv "/var/db/receipts/com.beyondtrust.$1.plist"
    else
        echo "Package '$1' is not installed"
    fi
}

pkg_likewise_cleanup()
{
    $RUN rm -rfv "/Library/Receipts/likewise.open*.pkg"
    $RUN rm -rfv "/var/db/receipts/com.likewise.open.pkg.bom"
    $RUN rm -rfv "/var/db/receipts/com.likewise.open.pkg.plist"

    $RUN rm -rfv "/Library/Receipts/likewisesoftware.open*.pkg"
    $RUN rm -rfv "/var/db/receipts/com.likewisesoftware.open.bom"
    $RUN rm -rfv "/var/db/receipts/com.likewisesoftware.open.plist"

    $RUN rm -rfv "/Library/Receipts/likewise.enterprise*.pkg"
    $RUN rm -rfv "/var/db/receipts/com.likewise.enterprise.pkg.bom"
    $RUN rm -rfv "/var/db/receipts/com.likewise.enterprise.pkg.plist"

    $RUN rm -rfv "/Library/Receipts/likewisesoftware.enterprise*.pkg"
    $RUN rm -rfv "/var/db/receipts/com.likewisesoftware.enterprise.bom"
    $RUN rm -rfv "/var/db/receipts/com.likewisesoftware.enterprise.plist"
}

launchctl_get_all_daemons()
{
    if [ -n "${HAVE_LAUNCHCTL_V2}" ]; then
        # Note that launchctl v2 has the daemon name in the third column.
        launchctl list | awk '{ print $3 }'
    else
        launchctl list
    fi
}

launchctl_get_daemons()
{
    launchctl_get_all_daemons | grep '^com\.beyondtrust\.pbis\.'
}

launchctl_view_daemons()
{
    # Note that launchctl v2 has the daemon name in the third column.
    launchctl list | grep 'com\.beyondtrust\.pbis\.'
}

launchctl_stop_daemons()
{
    launchctl_get_daemons | tr '\n' '\0' | xargs -0 -n1 $RUN launchctl stop
}

launchctl_unload_daemon()
{
    if [ -z "$1" ]; then
        echo "Missing daemon name."
        return 1
    fi
    true
    for _plist in \
        "/System/Library/LaunchDaemons/$1.plist" \
        "/Library/LaunchDaemons/$1.plist" \
        ; do
        if [ -f "${_plist}" ]; then
            launchctl unload "${_plist}"
        fi
    done
}

launchctl_unload_daemons()
{
    for daemon in `launchctl_get_daemons` ; do
        $RUN launchctl_unload_daemon "${daemon}"
    done
}

launchctl_delete_daemons()
{
    for _plist in \
        /System/Library/LaunchDaemons/com.beyondtrust.pbis.*.plist \
        /Library/LaunchDaemons/com.beyondtrust.pbis.*.plist \
        ; do
        $RUN rm -f "${_plist}"
    done
}

launchctl_delete_agents()
{
    for _plist in \
        /System/Library/LaunchAgents/com.beyondtrust.pbis.*.plist \
        /Library/LaunchAgents/com.beyondtrust.pbis.*.plist \
        /Library/LaunchAgents/com.likewise.*.plist \
        ; do
        $RUN rm -f "${_plist}"
    done
}

get_running_daemons()
{
    running=""
    for daemon in /opt/pbis/sbin/* ; do
        found=`ps -A -o "command" | egrep '^'"${daemon}"'( |$)' 2>/dev/null`
        if [ -n "${found}" ]; then
            running="${running} ${daemon}"
        fi
    done
    echo "${running}"
}

kill_daemons()
{
    daemons=`get_running_daemons`
    for daemon in ${daemons} ; do
        name=`basename "${daemon}"`
        echo "${name} is still running"
        # Note that -d will not actually kill (at least not on 10.5),
        # just print extra info.
        $RUN killall $1 -v -d "${name}"
        $RUN killall $1 "${name}"
    done
}

check_daemons()
{
    daemons=`get_running_daemons`
    if [ -n "${daemons}" ]; then
        if [ -n "$1" ]; then
            echo "WARNING: Some daemons are still running:"
            for daemon in ${daemons} ; do
                echo "    ${daemon}"
            done
        fi
        return 1
    else
        echo "All daemons are stopped."
        return 0
    fi
}

stop_daemons()
{
    check_daemons
    drc=$?
    if [ $drc -ne 0 ]; then
        launchctl_stop_daemons
        sleep 4
        check_daemons
        drc=$?
    fi
    if [ $drc -ne 0 ]; then
        kill_daemons
        sleep 2
        check_daemons
        drc=$?
    fi
    if [ $drc -ne 0 ]; then
        kill_daemons -9
        sleep 1
        check_daemons warn
    fi
}

usage()
{
    echo "usage: `basename $0` [--test]"
    echo ""
    echo "    This script completely uninstalls all Likewise products from the Mac."
    echo ""
    exit 1
}

main()
{
    RUN=""
    case "$1" in
        '')
            ;;
        --test)
            RUN="echo"
            ;;
        pkg_uninstall_rm)
            # Code for self-invocation
            RUN="$2"
            pkg_uninstall_rm "$3"
            exit $?
            ;;
        *)
            usage
            ;;
    esac

    gather_version_info
    exit_on_error $?

    echo "Uninstall started."

    if [ -f /opt/pbis/bin/domainjoin-cli ]; then
        echo "Leaving the domain..."
        $RUN /opt/pbis/bin/domainjoin-cli leave
        # In case leave fails, make sure that we always unconfigure PAM
        echo "Unconfiguring PAM..."
        $RUN /opt/pbis/bin/domainjoin-cli configure pam --disable
    fi

    stop_daemons
    launchctl_unload_daemons
    # Just to make sure everything is as expected
    launchctl_view_daemons
    launchctl_delete_daemons
    launchctl_delete_agents

    for module in /opt/pbis/lib/AuthMechanisms/*.so
    do
        mechanism=`basename "$module" .so`
        echo "Disabling auth mechanism '$mechanism'"
        $RUN /opt/pbis/libexec/authmechanism disable "$mechanism"
    done

    for pkgName in `pkg_list_pbis` ; do
        pkg_uninstall "${pkgName}"
    done

    pkg_likewise_cleanup

    $RUN rm -rfv /opt/pbis /etc/pbis /var/log/pbis /var/lib/pbis /var/cache/pbis "/Applications/Likewise Utilities" "/Applications/PBIS Utilities" "/Library/Security/SecurityAgentPlugins/com.likewise.bundle"

    for file in /etc/* /etc/pam.d/* /etc/krb5.conf /etc/hosts /etc/sshd_config /etc/ssh_config ; do
        orig="$file.lwidentity.orig"
        bak="$file.lwidentity.bak"
        if [ -f "$orig" ]; then
            if [ -f "$file" ]; then
                $RUN mv $orig $file
            else
                echo "Missing $file, but have $orig"
            fi
        fi
        if [ -f "$bak" ]; then
            $RUN rm -fv $bak
        fi
    done

    for plugin in LWIDSPLugIn LWEDSPlugIn LWDSPlugin ; do
        path="/System/Library/Frameworks/DirectoryService.framework/Resources/Plugins/${plugin}.dsplug"
        echo "Checking for link to DS plugin (${path})"
        if [ -h "${path}" ]; then
            $RUN rm -fv "${path}"
            echo "Removed symlink to DS plugin"
        fi
    done

    for plugin in LWIDSPLugIn LWEDSPlugIn LWDSPlugin ; do
        path="/Library/DirectoryServices/PlugIns/${plugin}.dsplug"
        echo "Checking for installed DS plugin (${path})"
        if [ -d "${path}" ]; then
            $RUN rm -r "${path}"
            echo "Removed DS plugin"
        fi
    done

    $RUN killall DirectoryService opendirectoryd dspluginhelperd

    echo "Uninstall completed."
}

main "$@"
exit 0
