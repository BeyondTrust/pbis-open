#!/bin/sh
# Implicit arguments
#  PREFIX - prefix of PowerBroker Identity Services installation
#  SERVICE_NAME - name of the service

LWSM="${PREFIX}/bin/lwsm"

PLATFORM=""
if [ -f /etc/init.d/functions ]; then
    . /etc/init.d/functions
    PLATFORM="REDHAT"
elif [ -f /etc/rc.status ]; then
    . /etc/rc.status
    PLATFORM="SUSE"
elif [ -f /etc/debian_version ]; then
    . /lib/lsb/init-functions
    PLATFORM="DEBIAN"
elif [ "`uname -s`" = 'AIX' ]; then
    PLATFORM="AIX"
elif [ "`uname -s`" = 'HP-UX' ]; then
    PLATFORM="HP-UX"
elif [ "`uname -s`" = 'SunOS' ]; then
    PLATFORM="SOLARIS"
elif [ "`uname -s`" = 'VMkernel' ]; then
    PLATFORM="ESXI"
elif [ "`uname`" = "FreeBSD" ]; then
    PLATFORM="FREEBSD"
else
    PLATFORM="UNKNOWN"
fi

service_description()
{
    echo "Likewise Open services"
}

service_start()
{
    status=""

    case "${PLATFORM}" in
        REDHAT)
            printf "%s" "Starting `service_description`: " 
            ${LWSM} -q autostart
            status=$?
            if [ $status -eq 0 ]
            then
                echo_success
                echo
            else
                echo_failure
                echo
            fi
            ;;
        SUSE)
            printf "%s" "Starting `service_description`"
            for i in 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15; do
                if [ -e "/var/lib/pbis/.lwsm" ]; then
                    break;
                fi
                sleep 1
            done
            ${LWSM} -q autostart
            status=$?
            if [ $status -eq 0 ]
            then
                rc_reset
                rc_status -v
            else
                rc_failed $status
                rc_status -v
            fi
            ;;
        DEBIAN)
            log_daemon_msg "Starting `service_description`"
            ${LWSM} -q autostart
            status=$?
            log_end_msg $status
            ;;
         HP-UX | SOLARIS | FREEBSD | ESXI | AIX)
            printf "%s" "Starting `service_description`"
            if [ -x /usr/sbin/svcadm ]; then
                # Don't do anything!
                status=0
            else
                ${LWSM} -q autostart
                status=$?
            fi

            if [ $status -eq 0 ]
            then
                echo " ...ok"
            else
                echo " ...failed"
            fi
            ;;
        UNKNOWN)
            ${LWSM} -q autostart
            status=$?
            ;;
    esac

    return $status
}

service_restart()
{
    status=""

    case "${PLATFORM}" in
        REDHAT)
            printf "%s" "Restarting `service_description` (not supported)"
            status=0
            echo_success
            ;;
        SUSE)
            printf "%s" "Restarting `service_description` (not supported)"
            status=0
            rc_reset
            rc_status -v
            ;;
        DEBIAN)
            log_daemon_msg "Restarting `service_description` (not supported)"
            status=0
            log_end_msg $status
            ;;
         HP-UX | SOLARIS | FREEBSD | ESXI | AIX)
            printf "%s" "Restarting `service_description` (not supported)"
            status=0
            echo " ...ok"
            ;;
        UNKNOWN)
            status=0
            ;;
    esac

    return $status
}

service_stop()
{
    status=""

    case "${PLATFORM}" in 
        REDHAT)
            printf "%s" "Stopping `service_description` (not supported) "
            status=0
            echo_success
            ;;
        SUSE)
            printf "%s" "Stopping `service_description` (not supported)"
            status=0
            rc_reset
            rc_status -v
            ;;
        DEBIAN)
            log_daemon_msg "Stopping `service_description` (not supported)"
            status=0
            log_end_msg $status
            ;;
        AIX | HP-UX | SOLARIS | FREEBSD | ESXI)
            printf "%s" "Stopping `service_description` (not supported)"
            status=0
            echo " ...ok"
            ;;
        UNKNOWN)
            status=0
            ;;
    esac

    return $status
}

service_refresh()
{
    case "${PLATFORM}" in
        REDHAT)
            printf "%s" "Refreshing `service_description $1` configuration"
            status=0
            echo_success
            ;;
        SUSE)
            printf "%s" "Refreshing `service_description $1` configuration"
            status=0
            rc_reset
            rc_status -v
            ;;
        DEBIAN)
            log_daemon_msg "Refreshing `service_description $1` configuration"
            status=0
            log_end_msg $status
            ;;
        AIX | HP-UX | SOLARIS | FREEBSD | ESXI)
            printf "%s" "Refreshing `service_description $1` configuration"
            status=0
            echo " ...ok"
            ;;
        UNKNOWN)
            status=0
            ;;
    esac

    return $status
}

if [ -f /etc/rc.subr ]; then
    . /etc/rc.subr
fi

case "$1" in
    start|faststart|forcestart|onestart)
        service_start "$SERVICE_NAME"
        exit $?
        ;;
    stop|faststop|forcestop|onestop)
        service_stop "$SERVICE_NAME"
        exit $?
        ;;
    status|faststatus|forcestatus|onestop)
        echo "not supported"
        exit 0
        ;;
    restart|fastrestart|forcerestart|onerestart)
        service_restart "$SERVICE_NAME"
        exit $?
        ;;
    refresh|reload)
        service_refresh "$SERVICE_NAME"
        exit $?
        ;;
    start_msg)
        echo "Starting `service_description`"
        exit 0
        ;;
    stop_msg)
        echo "Stopping `service_description` (not supported)"
        exit 0
        ;;
    rcvar|fastrcvar|forcercvar|onercvar)
        if type run_rc_command >/dev/null 2>&1; then
            # Looks like this is a FreeBSD based system.
            name=pbis
            rcvar="`set_rcvar`"
            start_cmd="service_start"
            stop_cmd="service_stop"
            eval "${name}_enable=YES"

            load_rc_config "$name"
            run_rc_command "$1" || exit $?
            exit 0
        fi
        ;;
    *)
        echo "Unrecognized command: $1"
        exit 1
        ;;
esac
