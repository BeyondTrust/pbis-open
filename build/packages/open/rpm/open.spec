# ex: set tabstop=4 expandtab shiftwidth=4:

%{!?Compat32: %define Compat32 0}

%define INIT_DIR /etc/init.d

%define _LIB lib
%ifarch x86_64
%define _LIB lib64
%endif

Name: 		%{RpmName}
Summary: 	Authentication services for Active Directory domains
Version: 	%{RpmVersion}
Release: 	%{RpmRelease}
License: 	Likewise Proprietary
URL: 		http://www.likewise.com/
Group: 		System Environment/Daemons
BuildRoot: 	%{buildRootDir}/%{name}-%{version}
Prereq: grep, sh-utils
AutoReq: no


%description
Likewise Open integrates Unix desktops and servers into an Active Directory environment by joining hosts to the domain and lets Unix applications and services authenticate MS Windows' users and groups via the PAM and Name Service Switch libraries.

%package devel
Summary: Likewise Software Development Kit
Group: Development/Libraries
Requires: likewise

%description devel
The likewise-open-devel package includes the development libraries and header files that supply the application programming interface for security and authentication.

%prep

%build

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
rsync -a %{PopulateRoot}/ ${RPM_BUILD_ROOT}/


%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root)
%dir /var/rpc
%ifarch x86_64
%{PrefixDir}/lib/*.so
%{PrefixDir}/lib/*.so.*
/lib/*.so
/lib/*.so.*
/lib/security/*.so
%endif
/usr/bin/domainjoin-cli
%{PrefixDir}/%{_lib}/*.so
%{PrefixDir}/%{_lib}/*.so.*
%{PrefixDir}/%{_lib}/lw-svcm/*.so
/%{_lib}/*.so
/%{_lib}/*.so.*
/%{_lib}/security/*.so
%{INIT_DIR}/*
/etc/likewise
%config(noreplace) /etc/likewise/gss/mech
%{PrefixDir}/bin
%{PrefixDir}/sbin
%{PrefixDir}/share
%{PrefixDir}/data
%{PrefixDir}/apache/2.0/*.so
%{PrefixDir}/apache/2.2/*.so
%{PrefixDir}/%{_lib}/gss/*.so
%{PrefixDir}/%{_lib}/engines/*.so
%{PrefixDir}/%{_lib}/krb5/plugins/preauth/*.so
%{PrefixDir}/%{_lib}/krb5/plugins/libkrb5/*.so
%{PrefixDir}/%{_lib}/krb5/plugins/libkrb5/*.so.*
%{PrefixDir}/%{_lib}/sasl2/lib*
%{PrefixDir}/ssl
/var/lib/likewise/lwconfig.xml
/var/lib/likewise/lwreport.xml

%define initScriptPathList %{INIT_DIR}/lwsmd %{INIT_DIR}/likewise
%define AdProviderPath /opt/likewise/%{_LIB}/liblsass_auth_provider_ad_open.so
%post
## chkconfig behaves differently on various updates of RHEL and SUSE
## So, we massage the init script according to the release, for now.
for daemon in %{initScriptPathList}; do
    if [ -x $daemon ]; then
        if grep "LWI_STARTUP_TYPE_" $daemon >/dev/null 2>&1; then
            daemon_new=${daemon}.new

            if [ -f /etc/redhat-release ]; then
                /bin/sed \
                    -e 's/^#LWI_STARTUP_TYPE_REDHAT\(.*\)$/\1/' \
                    -e'/^#LWI_STARTUP_TYPE_SUSE.*$/ d' \
                    -e'/^#LWI_STARTUP_TYPE_DEBIAN.*$/ d' \
                    -e'/^#LWI_STARTUP_TYPE_FREEBSD.*$/ d' \
                    $daemon > $daemon_new
            else
                /bin/sed \
                    -e 's/^#LWI_STARTUP_TYPE_SUSE\(.*\)$/\1/' \
                    -e '/^#LWI_STARTUP_TYPE_REDHAT.*$/ d' \
                    -e'/^#LWI_STARTUP_TYPE_DEBIAN.*$/ d' \
                    -e'/^#LWI_STARTUP_TYPE_FREEBSD.*$/ d' \
                    $daemon > $daemon_new
            fi
            mv $daemon_new $daemon
            chmod 0755 $daemon
        fi
    fi
done

DAEMONS_TO_HALT="lwmgmtd lwrdrd npcmuxd likewise-open centeris.com-lwiauthd centeris.com-gpagentd reapsysld lsassd lwiod netlogond dcerpcd eventlogd lwregd lwsmd"

UPGRADEDIR=/opt/likewise-upgrade

LOG=/var/log/likewise-open-install.log
TLOG=/tmp/LikewiseOpenTemp.txt

PKG_ARCH="__PKG_ARCH"
# Display to screen and log file with a blank line between entires.
log()
{
    echo $@
    echo
    echo $@ >> $LOG
    echo >> $LOG
}

# Display to screen and log file with no blank line.
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
run()
{
    eval "$@" > $TLOG 2>&1
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
    return $err
}

# Execute command.
# Log only to file.
run_quiet()
{
    eval "$@" > $TLOG 2>&1
    err=$?
    if [ $err -eq 0 ]; then
        echo "Success: $@" >> $LOG
    else
        echo "Error: $@ returned $err  (ignoring and continuing)" >> $LOG
    fi
    cat $TLOG >> $LOG
    echo >> $LOG
    rm -f $TLOG > /dev/null 2>&1
    return $err
}

# Execute command.
# If successful, note in log file.
# If not successful, note on screen and log file and then exit.
run_or_fail()
{
    eval "$@" > $TLOG 2>&1
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
    return $err
}

import_registry_configurations()
{
    REGIMPORT='/opt/likewise/bin/lwregshell import'

    log "Importing registry..."
    run_or_fail "$REGIMPORT /opt/likewise/share/config/dcerpcd.reg"
    run_or_fail "$REGIMPORT /opt/likewise/share/config/eventlogd.reg"
    run_or_fail "$REGIMPORT /opt/likewise/share/config/lsassd.reg"
    run_or_fail "$REGIMPORT /opt/likewise/share/config/privileges.reg"
    run_or_fail "$REGIMPORT /opt/likewise/share/config/accounts.reg"
    run_or_fail "$REGIMPORT /opt/likewise/share/config/lwiod.reg"
    run_or_fail "$REGIMPORT /opt/likewise/share/config/lwreg.reg"
    run_or_fail "$REGIMPORT /opt/likewise/share/config/netlogond.reg"
    run_or_fail "$REGIMPORT /opt/likewise/share/config/rdr.reg"
    run_or_fail "$REGIMPORT /opt/likewise/share/config/npfs.reg"
    run_or_fail "$REGIMPORT /opt/likewise/share/config/srv.reg"
    run_or_fail "$REGIMPORT /opt/likewise/share/config/pvfs.reg"
    run_or_fail "$REGIMPORT /opt/likewise/share/config/dfs.reg"
    run_or_fail "$REGIMPORT /opt/likewise/share/config/srvsvcd.reg"
    run_or_fail "$REGIMPORT /opt/likewise/share/config/nfs.reg"
    run_or_fail "$REGIMPORT /opt/likewise/share/config/reapsysl.reg"
}

determine_upgrade_type()
{
    PRESERVEDVERSIONFILE="${UPGRADEDIR}/VERSION"

    if [ -f $PRESERVEDVERSIONFILE ]; then
        run_or_fail "cat $PRESERVEDVERSIONFILE"
        if [ -n "`grep '^VERSION=5.0' $PRESERVEDVERSIONFILE`" ]; then
            UPGRADING_FROM_5_0123=1
            log "Upgrading from Likewise Identity Services Open 5.0"
        elif [ -n "`grep '^VERSION=5.1' $PRESERVEDVERSIONFILE`" ]; then
            UPGRADING_FROM_5_0123=1
            log "Upgrading from Likewise Identity Services Open 5.1."
        elif [ -n "`grep '^VERSION=5.2' $PRESERVEDVERSIONFILE`" ]; then
            UPGRADING_FROM_5_0123=1
            log "Upgrading from Likewise Identity Services Open 5.2."
        elif [ -n "`grep '^VERSION=5.3' $PRESERVEDVERSIONFILE`" ]; then
            UPGRADING_FROM_5_0123=1
            log "Upgrading from Likewise Identity Services Open 5.3."
        elif [ -n "`grep '^VERSION=6.0' $PRESERVEDVERSIONFILE`" ]; then
            UPGRADING_FROM_6_0=1
            log "Upgrading from Likewise Open 6.0."
        fi
    fi
}

import_5_0123_file()
{
    CONVERT="/opt/likewise/bin/conf2reg"
    REGIMPORT="/opt/likewise/bin/lwregshell import"

    COMMAND=$1
    SOURCE=$2
    # DEST is not necessary for some commands.
    DEST=$3

    if [ -f $SOURCE ]; then
        run_quiet "$CONVERT $COMMAND $SOURCE $DEST"
        if [ $? -ne 0 ]; then
            log "There was a problem converting $SOURCE. Please file a bug and attach $SOURCE."
            return 1
        fi

        if [ -n "$DEST" -a -f "$DEST" ]; then
            run_quiet "$REGIMPORT $DEST"
            if [ $? -ne 0 ]; then
                log "There was a problem converting $SOURCE. Please file a bug and attach $SOURCE and $DEST."
                return 1
            fi
        fi
    fi
    return 0
}

restore_5_0123_configuration()
{
    CONVERT="/opt/likewise/bin/conf2reg"

    if [ -z "$UPGRADING_FROM_5_0123" ]; then
        return 0
    fi

    import_5_0123_file "--lsass" "${UPGRADEDIR}/lsassd.conf" \
        "${UPGRADEDIR}/lsassd.conf.reg"

    import_5_0123_file "--netlogon" "${UPGRADEDIR}/netlogon.conf" \
        "${UPGRADEDIR}/netlogon.conf.reg"

    import_5_0123_file "--eventlog" "${UPGRADEDIR}/eventlogd.conf" \
        "${UPGRADEDIR}/eventlogd.conf.reg"

    import_5_0123_file "--pstore-sqlite" "${UPGRADEDIR}/pstore.db"
}

relocate_domain_separator()
{
    DomainSeparator=`/opt/likewise/bin/lwregshell list_values '[HKEY_THIS_MACHINE\Services\lsass\Parameters\Providers\ActiveDirectory]' | grep DomainSeparator | sed -e 's/ *[^ ]\+[ ]\+[^ ]\+[ ]\+"\([^ ]*\)"$/\1/'`

    if [ -n "${DomainSeparator}" ]; then
        if [ "$DomainSeparator" = "\\\\" ]; then
            DomainSeparator="\\"
        fi

        run_quiet /opt/likewise/bin/lwregshell set_value '[HKEY_THIS_MACHINE\Services\lsass\Parameters]' 'DomainSeparator' "$DomainSeparator"
    fi
}

relocate_space_replacement()
{
    SpaceReplacement=`/opt/likewise/bin/lwregshell list_values '[HKEY_THIS_MACHINE\Services\lsass\Parameters\Providers\ActiveDirectory]' | grep SpaceReplacement | sed -e 's/ *[^ ]\+[ ]\+[^ ]\+[ ]\+"\([^ ]*\)"$/\1/'`

    if [ -n "${SpaceReplacement}" ]; then
        run_quiet /opt/likewise/bin/lwregshell set_value '[HKEY_THIS_MACHINE\Services\lsass\Parameters]' 'SpaceReplacement' "$SpaceReplacement"
    fi
}

fix_60_registry()
{
    REGCLEANUP="/opt/likewise/bin/lwregshell cleanup"

    if [ -z "$UPGRADING_FROM_6_0" ]; then
        return 0
    fi

    # Migrate pstore entries from default to joined domain
    run "/opt/likewise/bin/regupgr61.sh --install"

    # Migrate some other entries
    relocate_domain_separator
    relocate_space_replacement

    run_or_fail "${REGCLEANUP} /opt/likewise/share/config/dcerpcd.reg"
    run_or_fail "${REGCLEANUP} /opt/likewise/share/config/eventlogd.reg"
    run_or_fail "${REGCLEANUP} /opt/likewise/share/config/lsassd.reg"
    run_or_fail "${REGCLEANUP} /opt/likewise/share/config/lwiod.reg"
    run_or_fail "${REGCLEANUP} /opt/likewise/share/config/lwreg.reg"
    run_or_fail "${REGCLEANUP} /opt/likewise/share/config/netlogond.reg"
}

switch_to_open_provider()
{
    _value='[HKEY_THIS_MACHINE\Services\lsass\Parameters\Providers\ActiveDirectory]'
    _path='%{AdProviderPath}'

    run_quiet "/opt/likewise/bin/lwregshell set_value $_value Path $_path"
}

wait_for_lwsm_shutdown()
{
    LWSMD_WAIT_TIME=$1

    while [ "$LWSMD_WAIT_TIME" -ne 0 ]; do
        sleep 1
        run_quiet "/opt/likewise/bin/lwsm list"
        if [ $? -ne 0 ]; then
            return 0
        fi
        LWSMD_WAIT_TIME=`expr $LWSMD_WAIT_TIME - 1`
    done

    return 1
}

postinstall()
{
    log "Package: Likewise Open postinstall begins (`date`)"
    log "Logging all operations to $LOG"

    run_or_fail "/opt/likewise/sbin/lwsmd --start-as-daemon --loglevel debug"

    determine_upgrade_type

    restore_5_0123_configuration

    import_registry_configurations

    fix_60_registry

    switch_to_open_provider

    run_or_fail "/opt/likewise/bin/lwsm shutdown"
    wait_for_lwsm_shutdown 60

    run "/sbin/chkconfig --add lwsmd"
    run "/sbin/chkconfig --add likewise"

    run "/etc/init.d/lwsmd start"
    run "/etc/init.d/likewise start"

    run "/opt/likewise/bin/domainjoin-cli configure --enable pam"
    run "/opt/likewise/bin/domainjoin-cli configure --enable nsswitch"

    run_quiet "rm -rf ${UPGRADEDIR}"

    log "Package: Likewise Open postinstall finished"
    exit 0
}

postinstall

%pre
DAEMONS_TO_HALT="lwmgmtd lwrdrd npcmuxd likewise-open centeris.com-lwiauthd centeris.com-gpagentd reapsysld lsassd lwiod netlogond dcerpcd eventlogd lwregd lwsmd"
UPGRADEDIR=/opt/likewise-upgrade

LOG=/var/log/likewise-open-install.log
TLOG=/tmp/LikewiseOpenTemp.txt

PKG_ARCH="__PKG_ARCH"

# Display to screen and log file with a blank line between entires.
log()
{
    echo $@
    echo
    echo $@ >> $LOG
    echo >> $LOG
}

# Display to screen and log file with no blank line.
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
run()
{
    eval "$@" > $TLOG 2>&1
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
    return $err
}

# Execute command.
# Log only to file.
run_quiet()
{
    eval "$@" > $TLOG 2>&1
    err=$?
    if [ $err -eq 0 ]; then
        echo "Success: $@" >> $LOG
    else
        echo "Error: $@ returned $err  (ignoring and continuing)" >> $LOG
    fi
    cat $TLOG >> $LOG
    echo >> $LOG
    rm -f $TLOG > /dev/null 2>&1
    return $err
}

# Execute command.
# If successful, note in log file.
# If not successful, note on screen and log file and then exit.
run_or_fail()
{
    eval "$@" > $TLOG 2>&1
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
    return $err
}

wait_for_lwsm_shutdown()
{
    LWSMD_WAIT_TIME=$1

    while [ "$LWSMD_WAIT_TIME" -ne 0 ]; do
        sleep 1

        run_quiet "/opt/likewise/bin/lwsm list"
        if [ $? -ne 0 ]; then
            return 0
        fi

        LWSMD_WAIT_TIME=`expr $LWSMD_WAIT_TIME - 1`
    done

    return 1
}

pre_upgrade()
{
    log "Package: Likewise Open [pre upgrade] begins (`date`)"

    run_quiet "/opt/likewise/bin/domainjoin-cli configure --disable pam"
    run_quiet "/opt/likewise/bin/domainjoin-cli configure --disable nsswitch"

    # Stop any Likewise daemons
    run_quiet "/opt/likewise/bin/lwsm shutdown"
    wait_for_lwsm_shutdown 60

    for daemon in $DAEMONS_TO_HALT
    do
        run_quiet "pkill -KILL -x $daemon"
    done

    log "Package: Likewise Open [pre upgrade] finished"
}

pre_install()
{
    log "Package: Likewise Open [pre install] begins (`date`)"

    logfile "Checking SELinux"
    if [ -x "/usr/sbin/selinuxenabled" -a -x "/usr/sbin/getenforce" ]; then
        logfile "/usr/sbin/selinuxenabled and /usr/sbin/getenforce are present"
        if /usr/sbin/selinuxenabled >/dev/null 2>&1; then
            logfile "selinuxenabled indicates SELinux is enabled"
            if /usr/sbin/getenforce 2>&1 | grep -v 'Permissive' >/dev/null 2>&1; then
                if [ -f /etc/selinux/config ]; then
                    log "SELinux found to be present, enabled, and enforcing.
SELinux must be disabled or set to permissive mode by editing the file
/etc/selinux/config and rebooting.
For instructions on how to edit the file to disable SELinux, see the SELinux man page."
                else
                    log "SELinux found to be present, enabled, and enforcing.
SELinux must be disabled or set to permissive mode.
Check your system's documentation for details."
                fi
                exit 1
            else
                logfile "getenforce indicates permissive (which is ok)"
            fi
        else
            logfile "selinuxenabled indicates SELinux is not enabled"
        fi
    fi

    for daemon in $DAEMONS_TO_HALT
    do
        run_quiet "pkill -KILL -x $daemon"
    done

    log "Package: Likewise Open [pre install] finished"
    exit 0
}

if [ $1 -eq 1 ]; then
    pre_install
else
    pre_upgrade
    pre_install
fi

%preun
DAEMONS_TO_HALT="lwmgmtd lwrdrd npcmuxd likewise-open centeris.com-lwiauthd centeris.com-gpagentd lwsmd lwregd netlogond lwiod dcerpcd eventlogd lsassd reapsysld"

UPGRADEDIR=/opt/likewise-upgrade

LOG=/var/log/likewise-open-install.log
TLOG=/tmp/LikewiseOpenTemp.txt

PKG_ARCH="__PKG_ARCH"

# Display to screen and log file with a blank line between entires.
log()
{
    echo $@
    echo
    echo $@ >> $LOG
    echo >> $LOG
}

# Display to screen and log file with no blank line.
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
run()
{
    eval "$@" > $TLOG 2>&1
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
    return $err
}

# Execute command.
# Log only to file.
run_quiet()
{
    eval "$@" > $TLOG 2>&1
    err=$?
    if [ $err -eq 0 ]; then
        echo "Success: $@" >> $LOG
    else
        echo "Error: $@ returned $err  (ignoring and continuing)" >> $LOG
    fi
    cat $TLOG >> $LOG
    echo >> $LOG
    rm -f $TLOG > /dev/null 2>&1
    return $err
}

# Execute command.
# If successful, note in log file.
# If not successful, note on screen and log file and then exit.
run_or_fail()
{
    eval "$@" > $TLOG 2>&1
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
    return $err
}

wait_for_lwsm_shutdown()
{
    LWSMD_WAIT_TIME=$1

    while [ "$LWSMD_WAIT_TIME" -ne 0 ]; do
        sleep 1

        run_quiet "/opt/likewise/bin/lwsm list"
        if [ $? -ne 0 ]; then
            return 0
        fi

        LWSMD_WAIT_TIME=`expr $LWSMD_WAIT_TIME - 1`
    done

    return 1
}

preuninstall_remove()
{
    log "Package: Likewise Open [preun remove] begins (`date`)"

    run_quiet /opt/likewise/bin/domainjoin-cli configure --disable pam
    run_quiet /opt/likewise/bin/domainjoin-cli configure --disable nsswitch

    run_quiet /opt/likewise/bin/domainjoin-cli configure \
                              --long `hostname --long` \
                              --short `hostname --short` \
                              --disable krb5

    # Stop all daemons; none should be needed anymore.
    run_quiet "/opt/likewise/bin/lwsm shutdown"
    wait_for_lwsm_shutdown 60
    for daemon in $DAEMONS_TO_HALT
    do
        run_quiet "pkill -KILL -x $daemon"
    done

    log "Package: Likewise Open [preun remove] finished"
    exit 0
}

if [ $1 -eq 0 ]; then
    preuninstall_remove
fi
exit 0


%files devel
%defattr(0644,root,root,0755)

%ifarch x86_64
%{PrefixDir}/lib/*.a
%{PrefixDir}/lib/*.la
/lib/*.a
/lib/*.la
/lib/security/*.la
%endif

%{PrefixDir}/%{_lib}/*.a
%{PrefixDir}/%{_lib}/*.la
%{PrefixDir}/%{_lib}/lw-svcm/*.a
%{PrefixDir}/%{_lib}/lw-svcm/*.la
/%{_lib}/*.a
/%{_lib}/*.la
/%{_lib}/security/*.la

%{PrefixDir}/apache/2.0/*.a
%{PrefixDir}/apache/2.2/*.a

%attr(0644,root,root) %{PrefixDir}/include/*
%attr(0644,root,root) %{PrefixDir}/share/man/*
%attr(0644,root,root) %{PrefixDir}/share/doc/*
%attr(0644,root,root) %{PrefixDir}/ssl/*
%attr(0644,root,root) %{PrefixDir}/%{_lib}/pkgconfig/*
%{PrefixDir}/%{_lib}/gss/*.a
%{PrefixDir}/%{_lib}/gss/*.la
%{PrefixDir}/%{_lib}/krb5/plugins/libkrb5/*.a
%{PrefixDir}/%{_lib}/krb5/plugins/libkrb5/*.la

%changelog
