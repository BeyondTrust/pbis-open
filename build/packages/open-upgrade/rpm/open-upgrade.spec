%define INIT_DIR /etc/init.d
%define _sysconfdir /etc

Name: 		%{RpmName}
Summary: 	Upgrade helper package for Likewise Open
Version: 	%{RpmVersion}
Release: 	%{RpmRelease}
License: 	Likewise Proprietary
URL: 		http://www.likewise.com/
Group: 		System Environment/Daemons
BuildRoot: 	%{buildRootDir}/%{name}-%{version}
Prereq: grep, sh-utils
AutoReq: no

%description
Likewise provides Active Directory authentication.

%prep

%build

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
rsync -a %{PopulateRoot}/ ${RPM_BUILD_ROOT}/

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root)

%if ! %{Compat32}
%pre
DAEMONS_TO_HALT="lwmgmtd lwrdrd npcmuxd likewise-open centeris.com-lwiauthd centeris.com-gpagentd lwsmd lwregd netlogond lwiod dcerpcd eventlogd lsassd"

LOG=/tmp/LikewiseOpen.log
TLOG=/tmp/LikewiseOpenTemp.txt

# Display to screen and log file.
log()
{
    echo $@
    echo
    echo $@ >> $LOG
    echo >> $LOG
}

_log()
{
    echo $@
    echo $@ >> $LOG
}

# Display to file.
logfile()
{
    echo $@ >> $LOG
    echo >> $LOG
}

# Execute command.
# If successful, note in log file.
# If not successful, note on screen and log file.
exec_log()
{
    $@ > $TLOG 2>&1
    err=$?
    if [ $err -eq 0 ]; then
        echo "Success: $@" >> $LOG
        cat $TLOG >> $LOG
        echo >> $LOG
    else
        _log "Error: $@ returned $err"
        _log `cat $TLOG`
        _log
    fi
    rm -f $TLOG > /dev/null 2>&1
}

# Execute command.
# Log only to file.
exec_logfile()
{
    $@ > $TLOG 2>&1
    err=$?
    if [ $err -eq 0 ]; then
        echo "Success: $@" >> $LOG
    else
        echo "Error: $@ returned $err  (ignoring and continuing)" >> $LOG
    fi
    cat $TLOG >> $LOG
    echo >> $LOG
    rm -f $TLOG > /dev/null 2>&1
}

# Execute command.
# If successful, note in log file.
# If not successful, note on screen and log file and then exit.
exec_log_exit()
{
    $@ > $TLOG 2>&1
    err=$?
    if [ $err -eq 0 ]; then
        echo "Success: $@" >> $LOG
        cat $TLOG >> $LOG
        echo >> $LOG
    else
        _log "Error: $@ returned $err  (aborting this script)"
        _log `cat $TLOG`
        _log
        rm -f $TLOG > /dev/null 2>&1
        exit 1
    fi
    rm -f $TLOG > /dev/null 2>&1
}

determine_upgrade_type()
{
    VERSIONFILE=/opt/likewise/data/VERSION
    if [ -f $VERSIONFILE ]; then
        UPGRADING=1
        UPGRADEDIR=/opt/likewise-upgrade
        exec_log_exit mkdir -p "${UPGRADEDIR}"

        exec_log_exit cat $VERSIONFILE
        if [ -n "`grep '^VERSION=5.0' $VERSIONFILE`" ]; then
            UPGRADING_FROM_5_0123=1
            log "Preserving 5.0 configuration in ${UPGRADEDIR}."
        elif [ -n "`grep '^VERSION=5.1' $VERSIONFILE`" ]; then
            UPGRADING_FROM_5_0123=1
            log "Preserving 5.1 configuration in ${UPGRADEDIR}."
        elif [ -n "`grep '^VERSION=5.2' $VERSIONFILE`" ]; then
            UPGRADING_FROM_5_0123=1
            log "Preserving 5.2 configuration in ${UPGRADEDIR}."
        elif [ -n "`grep '^VERSION=5.3' $VERSIONFILE`" ]; then
            UPGRADING_FROM_5_0123=1
            log "Preserving 5.3 configuration in ${UPGRADEDIR}."
        elif [ -n "`grep '^VERSION=5.4' $VERSIONFILE`" ]; then
            UPGRADING_FROM_5_4=1
            log "Preserving 5.4 configuration in ${UPGRADEDIR}."
        fi
    fi
}

preserve_5_0123_configuration()
{
    if [ -n "${UPGRADING_FROM_5_0123}" ]; then
        if [ -f "/etc/likewise/eventlogd.conf" ]; then
            exec_log_exit cp "/etc/likewise/eventlogd.conf" "${UPGRADEDIR}"
        fi

        if [ -f "/etc/likewise/lsassd.conf" ]; then
            exec_log_exit cp "/etc/likewise/lsassd.conf" "${UPGRADEDIR}"
        fi

        if [ -f "/etc/likewise/netlogon.conf" ]; then
            exec_log_exit cp "/etc/likewise/netlogon.conf" "${UPGRADEDIR}"
        fi

        if [ -f "/var/lib/likewise/db/pstore.db" ]; then
            exec_log_exit cp "/var/lib/likewise/db/pstore.db" "${UPGRADEDIR}"
            exec_log_exit chmod 700 "${UPGRADEDIR}/pstore.db"
        fi
    fi
}

preserve_5_4_configuration()
{
    if [ -n "${UPGRADING_FROM_5_4}" ]; then
        for file in dcerpcd.reg eventlogd.reg lsassd.reg lwiod.reg lwreg.reg netlogond.reg pstore.reg
        do
            if [ -f "/etc/likewise/${file}" ]; then
                exec_log_exit cp "/etc/likewise/${file}" "${UPGRADEDIR}"
            fi
        done
    fi
}


preinstall()
{
    log "Package: Likewise Open Upgrade Helper begins (`date`)"

    # Stop any Likewise daemon (process capture for debugging).
    logfile "Snapshot of processes before stopping daemons"
    exec_logfile "ps ax"
    for daemon in $DAEMONS_TO_HALT
    do
        if [ -x /etc/rc.d/$daemon ]; then
            exec_logfile "/etc/rc.d/$daemon stop"
        fi
        exec_logfile "pkill -TERM -x $daemon"
        exec_logfile "pkill -KILL -x $daemon"
    done
    logfile "Snapshot of processes after stopping daemons"
    exec_logfile ps ax

    determine_upgrade_type

    preserve_5_0123_configuration

    preserve_5_4_configuration

    log "Package: Likewise Open Upgrade Helper finished"
}

preinstall

%endif

%changelog
