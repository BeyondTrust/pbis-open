#!/bin/sh
# Implicit arguments
#  PREFIX - prefix of Likewise installation
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
    ${LWSM} info "$1" | grep '^Description:' | sed 's/^Description: //'
}

service_progname()
{
    basename "`${LWSM} info "$1" | grep '^Path:' | sed 's/^Path: //'`"
}

service_start()
{
    status=""

    case "${PLATFORM}" in
	REDHAT)
	    printf "%s" "Starting `service_progname $1`: " 
	    daemon ${LWSM} -q start "${1}"
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
	    printf "%s" "Starting `service_description $1`"
	    ${LWSM} -q start "${1}"
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
	    log_daemon_msg "Starting `service_description $1`: `service_progname $1`"
	    start-stop-daemon --start --exec ${LWSM} -- -q start "${1}"
	    status=$?
	    log_end_msg $status
	    ;;
	AIX)
	    printf "%s" "Starting `service_description $1`"
            if (lssrc -s dhcpcd | grep active >/dev/null); then
                # Wait up to 30 seconds for an ip address
                for i in `seq 30`; do
                    ifconfig -a | grep inet | grep -v 127.0.0 | grep -v 0.0.0.0 | grep -v ::1/0 >/dev/null && break
                    sleep 1
                done
            fi
            ${LWSM} -q start "${1}"
            status=$?
            ;;
	 HP-UX | SOLARIS | FREEBSD | ESXI)
            printf "%s" "Starting `service_description $1`"
            ${LWSM} -q start "${1}"
            status=$?
	    if [ $status -eq 0 ]
	    then
		echo " ...ok"
	    else
		echo " ...failed"
	    fi
	    ;;
        UNKNOWN)
            ${LWSM} -q start "${1}"
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
	    printf "%s" "Restarting `service_progname $1`: " 
	    daemon ${LWSM} -q restart "${1}"
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
	    printf "%s" "Restarting `service_description $1`"
	    ${LWSM} -q restart "${1}"
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
	    log_daemon_msg "Restarting `service_description $1`: `service_progname $1`"
	    start-stop-daemon --start --exec ${LWSM} -- -q restart "${1}"
	    status=$?
	    log_end_msg $status
	    ;;
	AIX)
	    printf "%s" "Restarting `service_description $1`"
            if (lssrc -s dhcpcd | grep active >/dev/null); then
                # Wait up to 30 seconds for an ip address
                for i in `seq 30`; do
                    ifconfig -a | grep inet | grep -v 127.0.0 | grep -v 0.0.0.0 | grep -v ::1/0 >/dev/null && break
                    sleep 1
                done
            fi
            ${LWSM} -q restart "${1}"
            status=$?
            ;;
	 HP-UX | SOLARIS | FREEBSD | ESXI)
            printf "%s" "Restarting `service_description $1`"
            ${LWSM} -q restart "${1}"
            status=$?
	    if [ $status -eq 0 ]
	    then
		echo " ...ok"
	    else
		echo " ...failed"
	    fi
	    ;;
        UNKNOWN)
            ${LWSM} -q restart "${1}"
            status=$?
            ;;
    esac

    return $status
}

service_stop()
{
    status=""

    case "${PLATFORM}" in 
        REDHAT)
            printf "%s" $"Stopping `service_progname $1`: "
            ${LWSM} -q stop "${1}"
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
            printf "%s" "Stopping `service_description $1`"
	    ${LWSM} -q stop "${1}"
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
            log_daemon_msg "Stopping `service_description $1`: `service_progname $1`"
	    ${LWSM} -q stop "${1}"
	    status=$?
            log_end_msg $status
            ;;
        AIX | HP-UX | SOLARIS | FREEBSD | ESXI)
            printf "%s" "Stopping `service_description $1`"
	    ${LWSM} -q stop "${1}"
	    status=$?
	    if [ $status -eq 0 ]
	    then
		echo " ...ok"
	    else
		echo " ...failed"
	    fi
	    ;;
        UNKNOWN)
	    ${LWSM} -q stop "${1}"
            status=$?
            ;;
    esac

    return $status
}

service_refresh()
{
    case "${PLATFORM}" in 
        REDHAT)
            printf "%s" $"Refreshing `service_description $1` configuration"
	    ${LWSM} -q refresh "$1"
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
            printf "%s" $"Refreshing `service_description $1` configuration"
	    ${LWSM} -q refresh "$1"
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
            log_daemon_msg "Refreshing `service_description $1` configuration"
	    ${LWSM} -q refresh "$1"
	    status=$?
            log_end_msg $status
            ;;
        AIX | HP-UX | SOLARIS | FREEBSD | ESXI)
            printf "%s" "Refreshing `service_description $1` configuration"
	    ${LWSM} -q refresh "$1"
	    status=$?
	    if [ $status -eq 0 ]
	    then
		echo " ...ok"
	    else
		echo " ...failed"
	    fi
	    ;;
	UNKNOWN)
	    ${LWSM} -q refresh "$1"
	    status=$?
	    ;;
    esac

    return $status
}

case "$1" in
    start)
	service_start "$SERVICE_NAME"
	exit $?
	;;
    stop)
	service_stop "$SERVICE_NAME"
	exit $?
	;;
    status)
	$LWSM status "$SERVICE_NAME"
	case "$?" in
	    0)
		exit 0
		;;
	    1)
		exit 3
		;;
	    2)
		exit 3
		;;
	    3)
		exit 3
		;;
	    4)
		exit 0
		;;
	    5)
		exit 2
		;;
	    *)
		exit 1
		;;
	esac
	;;
    info)
	$LWSM info "$SERVICE_NAME"
	exit $?
	;;
    restart)
	service_restart "$SERVICE_NAME"
	exit $?
	;;
    refresh|reload)
	service_refresh "$SERVICE_NAME"
	exit $?
	;;
    start_msg)
	echo "Starting `service_description $SERVICE_NAME`"
	exit 0
	;;
    stop_msg)
	echo "Stopping `service_description $SERVICE_NAME`"
	exit 0
	;;
    *)
	echo "Unrecognized command: $1"
	exit 1
	;;
esac
