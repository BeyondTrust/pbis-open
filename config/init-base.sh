#!/bin/sh

# Generic cross-platform init script logic
# Copyright (c) Likewise Software, Inc.
# All rights reserved.
#
# Authors: Jerry Carter (gcarter@likewisesoftware.com)
#          Kyle Stemen (kstemen@likewisesoftware.com)
#          Brian Koropoff (bkoropoff@likewisesoftware.com)

# Implict arguments to this script when sourced:
#
# PROG_DESC - English description of program/service
# PROG_BIN - path to the program binary
# PROG_ARGS - additional arguments to pass to program on startup
# PROG_ERR - file where error diagnostics are logged
# PIDFILE - file where pid is stored (or empty/unset if a pid file is not created)
# SCRIPTNAME - the name of the init script

## Have to set the path for HP-UX boot process
PATH=/sbin:/usr/sbin:/bin:/usr/bin:$PATH
export PATH

alias_replacement()
{
    # Simulates the alias builtin function. It does this by creating a function
    # with the name of what should be aliased. So if it was run like this:
    #   alias_replacement myecho='echo my'
    # Then the alias would be emulated like this:
    #   myecho()
    #   {
    #      echo my "$@"
    #   }
    if [ "$#" -ne 1 ]; then
        echo "alias takes 1 argument"
        return 1
    fi
    # This function is passed something like abc=xyz . The name variable gets
    # set to abc, and value gets set to xyz .
    name="`expr "$1" : '^\(.*\)='`"
    value="`expr "$1" : '.*=\(.*\)$'`"
    eval "$name() { $value \"\$@\"; }"
}

alias aliastest=echo
type aliastest 1>/dev/null 2>/dev/null
alias_works=$?

_test_alias() { false; }
alias _test_alias="true"
case a in a) _test_alias; alias_case_works=$?;; esac
eval '_test_alias >/dev/null 2>&1'
alias_eval_works=$?

if [ $alias_works -ne 0 -o $alias_case_works -ne 0 ]
then
    ( alias() { true; } ) >/dev/null 2>&1
    can_overwrite_builtins=$?
    unset alias >/dev/null 2>&1

    if [ $alias_eval_works -eq 0 -a $can_overwrite_builtins -ne 0 ]; then
        # This is HP-UX. The alias command only takes effect when the file
        # is read. Furthermore, HP-UX won't let us redefine shell
        # builtins as system functions. So we'll have to alias
        # whatever shell builtins we need to now, and reread the file. The
        # alias_replacement function can realias the shell builtins after
        # they are renamed.
        echo_rename()
        {
            echo "$@"
        }
        alias echo=echo_rename
        alias alias=alias_replacement
        # It would cause problems if these aliases are left around for
        # when this script is resourced.
        unalias _test_alias
        unalias aliastest
        # Resource this script
        . /opt/pbis/libexec/init-base.sh
        exit $?
    fi

    # This platform doesn't have a working alias (at all). It needs to be
    # replaced. This is primarily for Solaris and FreeBSD.
    alias()
    {
        alias_replacement "$@"
    }
fi


##
## Determine what platform we are on
##
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
elif [ "`uname`" = "FreeBSD" -o "`uname`" = "Isilon OneFS" ]; then
    PLATFORM="FREEBSD"
    extra_commands="reload"
    reload_cmd=daemon_reload
    status_cmd=daemon_status
    start_cmd=daemon_start
    stop_cmd=daemon_stop
else
    PLATFORM="UNKNOWN"
fi

if [ -f /etc/rc.subr ]; then
    . /etc/rc.subr
fi

if [ $PLATFORM = "HP-UX" -o $PLATFORM = "SOLARIS" -o $PLATFORM = "FREEBSD" ]; then
    LOCKFILE="/var/run/${SCRIPTNAME}.lock"
else
    LOCKFILE="/var/lock/subsys/${SCRIPTNAME}"
fi

type printf 1>/dev/null 2>/dev/null
if [ $? -ne 0 ]; then
    # Usually printf is a shell built in, but on HPUX it is a program located
    # at /bin/printf. During system startup and shutdown the path is only
    # /sbin, so we need to manually find printf
    if [ -x /bin/printf ]; then
        alias printf=/bin/printf
    else
        echo "WARNING: unable to find printf program"
    fi
fi

# echo_replacement emulates echo for all platforms using printf. printf is a
# shell builtin that exists on all platforms.
echo_replacement()
{
    if [ "$1" = "-n" ]; then
        shift;
        printf %s "$*"
    else
        printf %s\\n "$*"
    fi
}

# 'echo -n' works with bash, but not with sh on Solaris, HPUX, and AIX.
if [ "`echo -n`" = "-n" ]; then
    alias echo=echo_replacement
fi

seq_replacement()
{
    FIRST=1
    INCREMENT=1
    case "$#" in
        0)
            echo too few arguments
            return 1
            ;;
        1)
            LAST="$1"
            ;;
        2)
            FIRST="$1"
            LAST="$2"
            ;;
        3)
            FIRST="$1"
            INCREMENT="$2"
            LAST="$3"
            ;;
        *)
            echo too many arguments
            return 1
            ;;
    esac
    i="$FIRST"
    while [ "$i" -le "$LAST" ]; do
        echo "$i"
        i="`expr "$i" + "$INCREMENT"`"
    done
    return 0;
}

# seq doesn't exist on HPUX or FreeBSD
type seq 2>/dev/null 1>/dev/null
if [ $? -ne 0 ]; then
    alias seq=seq_replacement
fi

##
## small wrapper functions around distro specific calls
##

status_success() {
    case "${PLATFORM}" in 
        REDHAT)
            echo_success
            echo
            ;;
        SUSE)
            rc_reset
            rc_status -v
            ;;
        DEBIAN)
            ;;
        AIX | HP-UX | SOLARIS | FREEBSD | ESXI | UNKNOWN)
            echo "...ok"
            ;;
    esac
}

status_failed() {
    status=$1
    case "${PLATFORM}" in 
        REDHAT)
            echo_failure
            echo
            ;;
        SUSE)
            rc_failed $status
            rc_status -v
            ;;
        DEBIAN)
            ;;
        AIX | HP-UX | SOLARIS | ESXI | UNKNOWN)
            echo "...failed"
            ;;
    esac
}

print_status () {
    status=$1

    if [ $status = 0 ]; then
        status_success
    else
        status_failed $status
    fi
}

generic_status()
{
    #Uses return codes specified in
    # http://refspecs.freestandards.org/LSB_3.1.0/LSB-Core-generic/LSB-Core-generic/iniscrptact.html
    pids="`generic_pid`"
    #Take the list of pids and get just the last one
    pid=""
    for i in $pids; do
        pid="$i"
    done

    if [ -n "$pid" ]
    then
        #Is the pid valid?
        #Does the program with that pid match our program name?
        #HP-UX needs UNIX95 set to support the -o option
        if [ "${PLATFORM}" = "ESXI" ]; then
            if kill -0 $pid > /dev/null 2>&1 ; then
                # pid is valid. have to assume it's the right program
                return 0
            else
                return 1
            fi
        fi
        pid_comm="`UNIX95=1 ps -p "$pid" -o args= 2>/dev/null | awk '{print $1}'`"
        if [ "$pid_comm" = "<defunct>" ]; then
            #It is a zombie process
            return 4
        fi
        if [ "$pid_comm" = "${PROG_BIN}" ]; then
            #If the system keeps cmdline files, check it
            if [ -f /proc/${pid}/cmdline ]; then
                #We can't check the exe file because we may be looking
                #at a version of the program that has been overwritten
                grep -q ${PROG_BIN} /proc/${pid}/cmdline && return 0
            else
                return 0
            fi
        fi

        #Program is dead, but lock file exists
        [ -f "${LOCKFILE}" ] && return 2
        
        #Program is dead, but pid file exists
        return 1
    else
        return 3
    fi
}

generic_pid()
{
    if [ -n "${PIDFILE}" -a -f "${PIDFILE}" ]
    then
        cat "${PIDFILE}"
    else
        case "${PLATFORM}" in
            FREEBSD)
                pgrep -f "^${PROG_BIN}"
                ;;
            ESXI)
                ( ps | grep "^[0-9]* [0-9]* `basename ${PROG_BIN}` *${PROG_BIN}" | awk '{ print $1 };' | head -1 )
                ;;
            HP-UX)
                ( UNIX95= ps -e -o pid= -o args= | grep "^ *[0123456789]* *${PROG_BIN}" | awk '{ print $1 };' )
                ;;
            *)
                ( UNIX95=1; ps -e -o pid= -o args= | grep "^ *[0123456789]* *${PROG_BIN}" | awk '{ print $1 };' )
                ;;
        esac
    fi
}

check_load_path()
{
    libdir=/opt/pbis/lib
    if [ -x /opt/pbis/lib64 ]; then
        libdir=/opt/pbis/lib64
    fi
    for name in LD_LIBRARY_PATH LIBPATH SHLIB_PATH; do
        eval value=\$$name
        if [ -n "$value" ]; then
            expr "$value" : "^$libdir:" >/dev/null
            if [ $? -ne 0 ]; then
                if type logger >/dev/null 2>&1; then
                    logger -p daemon.error "LD_LIBRARY_PATH, LIBPATH, and SHLIB_PATH must be unset or list $libdir as the first directory. Likewise daemons will automatically unset the variable, but this variable still must be changed for the rest of your system. See the \"Requirements for the Agent\" section of the Likewise manual for more information."
                fi
                unset $name
                export $name
            fi
        fi
    done
    if [ -n "$LD_PRELOAD" ]; then
        if type logger >/dev/null 2>&1; then
            logger -p daemon.error "LD_PRELOAD must be unset. Likewise daemons will automatically unset the variable, but this variable still must be changed for the rest of your system. See the \"Requirements for the Agent\" section of the Likewise manual for more information."
        fi
        unset LD_PRELOAD
        export LD_PRELOAD
    fi
}

daemon_start() {

    if [ -f "${PROG_ERR}" ]; then
        /bin/rm -f $PROG_ERR;
    fi

    if [ -n "${STARTHOOK}" ]; then
        ${STARTHOOK}
    fi

    check_load_path

    case "${PLATFORM}" in 
        REDHAT)
            echo -n $"Starting `basename ${PROG_BIN}`: "
            daemon ${PROG_BIN} ${PROG_ARGS}
            status=$?
            ;;
        SUSE)
            echo -n "Starting $PROG_DESC"
            startproc ${PROG_BIN} ${PROG_ARGS}
            status=$?
            ;;
        DEBIAN)
            log_daemon_msg "Starting $PROG_DESC: `basename $PROG_BIN`"
            start-stop-daemon --start --exec ${PROG_BIN} -- ${PROG_ARGS}
            status=$?
            log_end_msg $status
            ;;
        AIX)
            echo -n "Starting $PROG_DESC"
            if (lssrc -s dhcpcd | grep active >/dev/null); then
                # Wait up to 30 seconds for an ip address
                for i in `seq 30`; do
                    ifconfig -a | grep inet | grep -v 127.0.0 | grep -v 0.0.0.0 | grep -v ::1/0 >/dev/null && break
                    sleep 1
                done
            fi
            ${PROG_BIN} ${PROG_ARGS}
            status=$?
            if [ $status -eq 0 ]; then
                status=1
                for i in `seq 5`; do
                    #Did the program start?
                    generic_status
                    status=$?
                    [ $status -eq 0 ] && break
                    sleep 1
                done
            fi
            ;;
        HP-UX | SOLARIS | FREEBSD | ESXI)
            echo -n "Starting $PROG_DESC"
            if type svcadm >/dev/null 2>&1 ; then
                # Use the solaris service manager

                # This will start the program again if it was in maintenance
                # mode.
                svcadm clear "$SCRIPTNAME" 2>/dev/null
                # This will start the program again if it was disabled.
                svcadm enable "$SCRIPTNAME"
                status=$?
            else
                ${PROG_BIN} ${PROG_ARGS}
                status=$?
            fi
            if [ $status -eq 0 ]; then
                status=1
                for i in `seq 5`; do
                    #Did the program start?
                    generic_status
                    status=$?
                    [ $status -eq 0 ] && break
                    sleep 1
                done
            fi
            ;;
        UNKNOWN)
            ${PROG_BIN} ${PROG_ARGS}
            status=$?
            ;;
    esac

    if [ -n "${POSTSTARTHOOK}" ]; then
        ${POSTSTARTHOOK}
    fi

    [ $status = 0 ] && [ ${PLATFORM} != "DEBIAN" ] && touch ${LOCKFILE}
    return $status
}

daemon_stop() {
    case "${PLATFORM}" in 
        DEBIAN)
            log_daemon_msg "Stopping $PROG_DESC: `basename $PROG_BIN`"
            status=1
            #only try to stop the daemon if it is running
            if generic_status; then
                kill -TERM "`generic_pid`"
                # Forget our pidfile since it will now be invalid
                # if is still present.  This means generic_status
                # will fall back on ps to see if the daemon is still
                # running.  This is important on LinuxThreads-based
                # systems where other threads can still be in the process
                # of shutting down even when the main thread has exited
                PIDFILE=""
                #Wait up to 180 seconds for the program to end
                for i in `seq 180`; do
                    #Did the program end?
                    generic_status
                    # Make sure the agent is not running and is not a zombie
                    # process.
                    [ $? -ne 0 -a $? -ne 4 ] && status=0 && break
                    # use the following line instead after bug 3634 is fixed
                    #[ $? -eq 3 -o $? -eq 2 ] && status=0 && break
                    sleep 1
                done
            fi
            log_end_msg $status
            ;;
        REDHAT | SUSE | AIX | HP-UX | SOLARIS | FREEBSD | ESXI)
            echo -n "Stopping $PROG_DESC"
            status=1
            #only try to stop the daemon if it is running
            if type svcadm >/dev/null 2>&1 && generic_status; then
                # Use the solaris service manager
                svcadm disable "$SCRIPTNAME"

                #Wait up to 180 seconds for the program to end
                for i in `seq 180`; do
                    #Did the program end?
                    generic_status
                    # Make sure the agent is not running and is not a zombie
                    # process.
                    [ $? -ne 0 -a $? -ne 4 ] && status=0 && break
                    # use the following line instead after bug 3634 is fixed
                    #[ $? -eq 3 -o $? -eq 2 ] && status=0 && break
                    sleep 1
                done
            fi
            if generic_status; then
                # svcadm does not exist, or it was unable to stop the program
                pid="`generic_pid`"
                kill -TERM $pid

                #Wait up to 180 seconds for the program to end
                for i in `seq 180`; do
                    #Did the program end?
                    generic_status
                    # Make sure the agent is not running and is not a zombie
                    # process.
                    [ $? -ne 0 -a $? -ne 4 ] && status=0 && break
                    # use the following line instead after bug 3634 is fixed
                    #[ $? -eq 3 -o $? -eq 2 ] && status=0 && break
                    sleep 1
                done
            fi
            ;;
        UNKNOWN)
            killall -TERM `basename ${PROG_BIN}`
            status=$?
            ;;
    esac

    [ $status = 0 ] && /bin/rm -f ${LOCKFILE}
    return $status
}

daemon_reload()
{
    case "${PLATFORM}" in 
        REDHAT | SUSE | FREEBSD | ESXI)
            echo -n $"Reloading ${PROG_DESC} configuration"
	    killall -HUP "`basename ${PROG_BIN}`"
            status=$?
            ;;
        DEBIAN)
            log_daemon_msg "Reloading $PROG_DESC configuration"
	    killall -HUP "`basename ${PROG_BIN}`"
	    status=$?
            log_end_msg $status
            ;;
        AIX | HP-UX | SOLARIS | UNKNOWN)
            echo -n "Stopping $PROG_DESC"
	    generic_killall -HUP
	    status=$?
            ;;
    esac

    return $status
}

daemon_status() {
    case "${PLATFORM}" in
        REDHAT)
            status ${PROG_BIN}
            ;;
        SUSE)
            checkproc ${PROG_BIN}
            rc_status -v
            ;;
        AIX | HP-UX | SOLARIS | DEBIAN | FREEBSD | ESXI)
            generic_status
            status=$?
            case "$status" in
                0)
                    echo "running";
                    ;;
                1)
                    echo "stopped";
                    #Use the bellow line instead after bug 3634 is fixed
                    #echo "died leaving pid file";
                    ;;
                2)
                    echo "died leaving lock file";
                    ;;
                3)
                    echo "stopped";
                    ;;
            esac
            return $status
        ;;
        UNKNOWN)
            echo "Not implemented."
            ;;
    esac
}

#Sends the signal specified by the first parameter to all instances
#of $PROG_BIN
generic_killall()
{
    #This gets the list of all current running instances of PROG_BIN,
    #but it can't tell which directory the program was run from. If
    #cmdline is available in the proc pseudo-filesystem, we'll verify
    #the pids.
    unverified_pids=`generic_pid`
    pids=
    for pid in $unverified_pids; do
        #If the system keeps cmdline files, check them
        if [ -f /proc/${pid}/cmdline ]; then
            #We can't check the exe file because we may be looking
            #at a version of the program that has been overwritten
            grep -q ${AUTHD_BIN} /proc/${pid}/cmdline && pids="$pids $pid"
        else
            pids="$pids $pid"
        fi
    done

    for pid in $pids; do
        [ ! -z "`UNIX95=1 ps -p $pid -o pid=`" ] && kill "$1" $pid
    done

    #The manpage of killall says it returns 0 if at least one process is
    #killed. I can't get it to return anything other than 0. Even if no
    #processes die with SIGTERM. We'll just return 0 here too
    return 0
}

####################################################################
## Main init script code
##

if type run_rc_command >/dev/null 2>&1; then
    # Looks like this is a FreeBSD based system. We should use their arg
    # parsing instead of our own.
    name="$SCRIPTNAME"
    rcvar="`set_rcvar`"
    command="$PROG_BIN"
    command_args="$PROG_ARGS"
    pidfile="$PIDFILE"
    eval "${name}_enable=YES"

    load_rc_config "$name"
    [ "$1" = "start" -a -n "${STARTHOOK}" ] && ${STARTHOOK}
    check_load_path
    run_rc_command "$1" || exit $?
    [ "$1" = "start" -a -n "${POSTSTARTHOOK}" ] && ${POSTSTARTHOOK}
    exit 0
fi

case "$1" in 
    start)
        daemon_start
        ret=$?
        print_status $ret
        ;;

    stop)
        daemon_stop
        ret=$?
        print_status $ret
        ;;

    # HP-UX calls this
    stop_msg)
        echo "Stopping $PROG_DESC"
        exit 0
        ;;

    # HP-UX calls this
    start_msg)
        echo "Starting $PROG_DESC"
        exit 0
        ;;

    restart)
        daemon_stop
        ret=$?
        print_status $ret
        daemon_start
        ret=$?
        print_status $ret
        ;;
        
    status)
        daemon_status
        ;;

    reload)
	# Ignore SIGHUP since killall could send it to us as well
	trap '' HUP
	daemon_reload
	ret=$?
	print_status $ret
        ;;
    *)
        echo "Usage: $0 {start|stop|restart|status|reload}"
        exit 1
        ;;
esac

