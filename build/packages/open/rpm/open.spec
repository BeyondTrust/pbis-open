# ex: set tabstop=4 expandtab shiftwidth=4:

%{!?Compat32: %define Compat32 0}

%define INIT_DIR /etc/init.d
%define _sysconfdir /etc

%if %{Compat32}
%define _lib lib
%endif

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

# Make sure files are not included in both likewise and likewise-compat
%if %{Compat32}
rm -rf $RPM_BUILD_ROOT/%{PrefixDir}/include
rm -rf $RPM_BUILD_ROOT/%{PrefixDir}/share
%endif


%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root)
%dir /var/rpc
%{PrefixDir}/%{_lib}/*.so
%{PrefixDir}/%{_lib}/*.so.*
/%{_lib}/*.so
/%{_lib}/*.so.*
/%{_lib}/security/*.so
%if !%{Compat32}
%{INIT_DIR}/*
/etc/likewise
%config(noreplace) /etc/likewise/gss/mech
%{PrefixDir}/bin
%{PrefixDir}/sbin
%{PrefixDir}/share
%{PrefixDir}/data
#%{PrefixDir}/apache
%{PrefixDir}/%{_lib}/gss/*.so
%{PrefixDir}/%{_lib}/engines/*.so
%{PrefixDir}/%{_lib}/lwsm-loader/*.so
%{PrefixDir}/%{_lib}/krb5/plugins/preauth/*.so
%{PrefixDir}/%{_lib}/krb5/plugins/libkrb5/*.so
%{PrefixDir}/%{_lib}/krb5/plugins/libkrb5/*.so.*
%{PrefixDir}/%{_lib}/sasl2/lib*
%{PrefixDir}/ssl
/var/lib/likewise/lwconfig.xml
/var/lib/likewise/lwreport.xml
%endif

%if ! %{Compat32}
%define initScriptPathList %{INIT_DIR}/dcerpcd %{INIT_DIR}/eventlogd %{INIT_DIR}/lsassd %{INIT_DIR}/lwiod %{INIT_DIR}/lwregd %{INIT_DIR}/lwsmd %{INIT_DIR}/netlogond
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
                    $daemon > $daemon_new
            else
                /bin/sed \
                    -e 's/^#LWI_STARTUP_TYPE_SUSE\(.*\)$/\1/' \
                    -e '/^#LWI_STARTUP_TYPE_REDHAT.*$/ d' \
                    $daemon > $daemon_new
            fi
            mv $daemon_new $daemon
            chmod 0755 $daemon
        fi
    fi
done

DAEMONS_TO_HALT="lwmgmtd lwrdrd npcmuxd likewise-open centeris.com-lwiauthd centeris.com-gpagentd lwsmd lwregd netlogond lwiod dcerpcd eventlogd lsassd"

UPGRADEDIR=/opt/likewise-upgrade

LOG=/tmp/LikewiseOpen.log
TLOG=/tmp/LikewiseOpenTemp.txt

PKG_ARCH="__PKG_ARCH"

# Display to screen and log file with a blank line.
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
    return $err
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
    return $err
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
    return $err
}

import_registry_configurations()
{
    REGUPGRADE='/opt/likewise/bin/lwregshell import'

    log "Importing registry configurations"
    exec_log_exit "$REGUPGRADE /opt/likewise/share/config/dcerpcd.reg"
    exec_log_exit "$REGUPGRADE /opt/likewise/share/config/eventlogd.reg"
    exec_log_exit "$REGUPGRADE /opt/likewise/share/config/lsassd.reg"
    exec_log_exit "$REGUPGRADE /opt/likewise/share/config/lwiod.reg"
    exec_log_exit "$REGUPGRADE /opt/likewise/share/config/lwreg.reg"
    exec_log_exit "$REGUPGRADE /opt/likewise/share/config/netlogond.reg"
    exec_log_exit "$REGUPGRADE /opt/likewise/share/config/pstore.reg"
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
        exec_log "$CONVERT $COMMAND $SOURCE $DEST"
        if [ $? -ne 0 ]; then
            log "Please file a bug and attach $SOURCE."
            return 1
        fi

        if [ -n "$DEST" -a -f "$DEST" ]; then
            exec_log "$REGIMPORT $DEST"
            if [ $? -ne 0 ]; then
                log "Please file a bug and attach $SOURCE and $DEST."
                return 1
            fi
        fi
    fi
    return 0
}

restore_5_0123_configuration()
{
    CONVERT="/opt/likewise/bin/conf2reg"

    if [ ! -d "${UPGRADEDIR}" ]; then
        return 0;
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
        if [ "${DomainSeparator}" = '\\' ]; then
            DomainSeparator='\'
# Balance quotes: '
        fi

        exec_logfile /opt/likewise/bin/lwregshell set_value '[HKEY_THIS_MACHINE\Services\lsass\Parameters]' 'DomainSeparator' "${DomainSeparator}"
    fi
}

relocate_space_replacement()
{
    SpaceReplacement=`/opt/likewise/bin/lwregshell list_values '[HKEY_THIS_MACHINE\Services\lsass\Parameters\Providers\ActiveDirectory]' | grep SpaceReplacement | sed -e 's/ *[^ ]\+[ ]\+[^ ]\+[ ]\+"\([^ ]*\)"$/\1/'`

    if [ -n "${SpaceReplacement}" ]; then
        exec_logfile /opt/likewise/bin/lwregshell set_value '[HKEY_THIS_MACHINE\Services\lsass\Parameters]' 'SpaceReplacement' "$SpaceReplacement"
    fi
}

# Assumes the registry is not empty
migrate_pstore()
{
    tmpreg=/tmp/regup-$$.txt
    LWREGSHELL=/opt/likewise/bin/lwregshell
    PSTORE_UPGRADE=/opt/likewise/bin/psupgrade

    # Export the existing registry, in legacy format
    exec_log_exit $LWREGSHELL export --legacy $tmpreg
    if [ ! -s $tmpreg ]; then
        log "Could not export registry."
        exit 1
    fi

    # "Rename" relevant pstore entries to new registry location
    $PSTORE_UPGRADE $tmpreg > ${tmpreg}.out
    if [ ! -s ${tmpreg}.out ]; then
        # Nothing to move/rename
        exec_logfile "rm -f $tmpreg"
        exec_logfile "rm -f ${tmpreg}.out"
        return
    fi

    # Import renamed pstore entries
    exec_log_exit "$LWREGSHELL import ${tmpreg}.out"
    exec_logfile "rm -f ${tmpreg}.out"

    # Clear out old pstore entries
    if [ `grep -c '\[HKEY_THIS_MACHINE\Services\lsass\Parameters\Providers\ActiveDirectory\Pstore\Default\]' $tmpreg` -gt 0 ]; then
      exec_logfile $LWREGSHELL delete_tree '[HKEY_THIS_MACHINE\Services\lsass\Parameters\Providers\ActiveDirectory\Pstore\Default]'
    fi

    # Remove values with identical default attributes from registry
    rm -f $tmpreg
}


fix_60_registry()
{
    REGCLEANUP="/opt/likewise/bin/lwregshell cleanup"

    migrate_pstore

    exec_log_exit "${REGCLEANUP} /opt/likewise/share/config/dcerpcd.reg"
    exec_log_exit "${REGCLEANUP} /opt/likewise/share/config/eventlogd.reg"
    exec_log_exit "${REGCLEANUP} /opt/likewise/share/config/lsassd.reg"
    exec_log_exit "${REGCLEANUP} /opt/likewise/share/config/lwiod.reg"
    exec_log_exit "${REGCLEANUP} /opt/likewise/share/config/lwreg.reg"
    exec_log_exit "${REGCLEANUP} /opt/likewise/share/config/netlogond.reg"
    exec_log_exit "${REGCLEANUP} /opt/likewise/share/config/pstore.reg"
}

switch_to_open_provider()
{
    _value='[HKEY_THIS_MACHINE\Services\lsass\Parameters\Providers\ActiveDirectory]'
    _path='%{AdProviderPath}'

    exec_log "/opt/likewise/bin/lwregshell set_value $_value Path $_path"
}

postinstall()
{
    log "Package: Likewise Open [post install] begins (`date`)"

    exec_log_exit "/opt/likewise/sbin/lwsmd --start-as-daemon"

    restore_5_0123_configuration

    import_registry_configurations

    relocate_domain_separator

    relocate_space_replacement

    switch_to_open_provider

    fix_60_registry

    exec_log_exit "/etc/init.d/lwsmd stop"

    exec_log_exit "/etc/init.d/lwsmd start"

    exec_log_exit "/opt/likewise/bin/domainjoin-cli query"

    exec_logfile "/opt/likewise/bin/domainjoin-cli configure --enable pam"

    exec_logfile "/opt/likewise/bin/domainjoin-cli configure --enable nsswitch"

    exec_logfile "rm -rf ${UPGRADEDIR}"

    log "Package: Likewise Open [post install] finished"
}

postinstall

%pre
DAEMONS_TO_HALT="lwmgmtd lwrdrd npcmuxd likewise-open centeris.com-lwiauthd centeris.com-gpagentd lwsmd lwregd netlogond lwiod dcerpcd eventlogd lsassd"

UPGRADEDIR=/opt/likewise-upgrade

LOG=/tmp/LikewiseOpen.log
TLOG=/tmp/LikewiseOpenTemp.txt

PKG_ARCH="__PKG_ARCH"

# Display to screen and log file with a blank line.
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
    return $err
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
    return $err
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
    return $err
}

pre_upgrade()
{
    log "Package: Likewise Open [pre upgrade] begins (`date`)"

    exec_logfile /opt/likewise/bin/domainjoin-cli configure --disable pam

    exec_logfile /opt/likewise/bin/domainjoin-cli configure --disable nsswitch
    exec_logfile /opt/likewise/bin/lwsm stop lsass
    exec_logfile /opt/likewise/bin/lwsm stop netlogon
    exec_logfile /opt/likewise/bin/lwsm stop lwio
    exec_logfile /opt/likewise/bin/lwsm stop eventlog
    exec_logfile /opt/likewise/bin/lwsm stop dcerpc
    exec_logfile /etc/init.d/lwsmd stop

    for daemon in $DAEMONS_TO_HALT
    do
        exec_logfile "pkill -KILL -x $daemon"
    done

    log "Package: Likewise Open [pre upgrade] finished"
}

pre_install()
{
    log "Package: Likewise Open [pre install] begins (`date`)"

    for daemon in $DAEMONS_TO_HALT
    do
        exec_logfile "pkill -KILL -x $daemon"
    done

    log "Package: Likewise Open [pre install] finished"
}

if [ $1 -eq 1 ]; then
    pre_install
else
    pre_upgrade
    pre_install
fi

%preun
DAEMONS_TO_HALT="lwmgmtd lwrdrd npcmuxd likewise-open centeris.com-lwiauthd centeris.com-gpagentd lwsmd lwregd netlogond lwiod dcerpcd eventlogd lsassd"

UPGRADEDIR=/opt/likewise-upgrade

LOG=/tmp/LikewiseOpen.log
TLOG=/tmp/LikewiseOpenTemp.txt

PKG_ARCH="__PKG_ARCH"

# Display to screen and log file with a blank line.
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
    return $err
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
    return $err
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
    return $err
}

preuninstall_remove()
{
    log "Package: Likewise Open [preun remove] begins (`date`)"

    exec_logfile /opt/likewise/bin/domainjoin-cli configure --disable pam

    exec_logfile /opt/likewise/bin/domainjoin-cli configure --disable nsswitch

    exec_logfile /opt/likewise/bin/domainjoin-cli configure --disable ssh

    exec_logfile /opt/likewise/bin/domainjoin-cli configure \
                              --long `hostname --long` \
                              --short `hostname --short` \
                              --disable krb5

    # Stop all daemons; none should be needed anymore.
    exec_logfile /etc/init.d/lwsmd stop

    for daemon in $DAEMONS_TO_HALT
    do
        exec_logfile "pkill -KILL -x $daemon"
    done

    log "Package: Likewise Open [preun remove] finished"
}

if [ $1 -eq 0 ]; then
    preuninstall_remove
fi
%endif

%files devel
%defattr(0644,root,root,0755)

%{PrefixDir}/%{_lib}/*.a
%{PrefixDir}/%{_lib}/*.la
/%{_lib}/*.a
/%{_lib}/*.la
/%{_lib}/security/*.la

%if ! %{Compat32}
%attr(0644,root,root) %{PrefixDir}/include/*
%attr(0644,root,root) %{PrefixDir}/share/man/*
%attr(0644,root,root) %{PrefixDir}/share/doc/*
%attr(0644,root,root) %{PrefixDir}/ssl/*
%attr(0644,root,root) %{PrefixDir}/%{_lib}/pkgconfig/*
%{PrefixDir}/%{_lib}/lwsm-loader/*.la
%{PrefixDir}/%{_lib}/gss/*.a
%{PrefixDir}/%{_lib}/gss/*.la
%{PrefixDir}/%{_lib}/krb5/plugins/libkrb5/*.a
%{PrefixDir}/%{_lib}/krb5/plugins/libkrb5/*.la
%endif

%changelog
