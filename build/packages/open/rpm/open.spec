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
Summary: 	Likewise Open is amazing
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

%package devel
Summary: Likewise Software Development Kit
Group: Development/Libraries
Requires: likewise

%description devel
The likewise-devel package includes the development libraries and header files that supply the application programming interface for security and
authentication.

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

import_registry_configuration()
{
    exec_log_exit \
        "/opt/likewise/bin/lwregshell upgrade /opt/likewise/share/config/$1.reg"
}

import_registry_configurations()
{
    log "Importing registry configurations"
    import_registry_configuration dcerpcd
    import_registry_configuration eventlogd
    import_registry_configuration lsassd
    import_registry_configuration lwiod
    import_registry_configuration lwreg
    import_registry_configuration netlogond
    import_registry_configuration pstore
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
        exec_log $CONVERT $COMMAND $SOURCE $DEST > /dev/null 2>&1
        if [ $? -ne 0 ]; then
            return 1
        fi

        if [ -n "$DEST" -a -f "$DEST" ]; then
            exec_log $REGIMPORT $DEST > /dev/null 2>&1
            if [ $? -ne 0 ]; then
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

fix_old_registry()
{
    DomainSeparator=`/opt/likewise/bin/lwregshell list_values '[HKEY_THIS_MACHINE\Services\lsass\Parameters\Providers\ActiveDirectory]' | grep DomainSeparator | sed -e 's/ *[^ ]\+[ ]\+[^ ]\+[ ]\+"\([^ ]*\)"$/\1/'`
    SpaceReplacement=`/opt/likewise/bin/lwregshell list_values '[HKEY_THIS_MACHINE\Services\lsass\Parameters\Providers\ActiveDirectory]' | grep SpaceReplacement | sed -e 's/ *[^ ]\+[ ]\+[^ ]\+[ ]\+"\([^ ]*\)"$/\1/'`
    if [ -n "${DomainSeparator}" ]; then
        if [ "$DomainSeparator" = "\\\\" ]; then
            DomainSeparator="\\"
        fi
        /opt/likewise/bin/lwregshell set_value '[HKEY_THIS_MACHINE\Services\lsass\Parameters]' 'DomainSeparator' "$DomainSeparator"
    fi
    if [ -n "${SpaceReplacement}" ]; then
        /opt/likewise/bin/lwregshell set_value '[HKEY_THIS_MACHINE\Services\lsass\Parameters]' 'SpaceReplacement' "$SpaceReplacement"
    fi
}

switch_to_open_provider()
{
    _value='[HKEY_THIS_MACHINE\Services\lsass\Parameters\Providers\ActiveDirectory]'
    _path='%{AdProviderPath}'

    exec_log "/opt/likewise/bin/lwregshell set_value $_value Path $_path"
}

postinstall()
{
    log "Package: Likewise Open post-install begins (`date`)"

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
    exec_logfile "ps ax"

    exec_log_exit "/opt/likewise/sbin/lwsmd --start-as-daemon --disable-autostart"

    restore_5_0123_configuration

    import_registry_configurations

    fix_old_registry

    switch_to_open_provider

    exec_log_exit "/etc/init.d/lwsmd stop"

    exec_log_exit "/etc/init.d/lwsmd start"

    exec_log_exit "/opt/likewise/bin/domainjoin-cli query"

    log "Package: Likewise Open post-install finished"
}

if [ $1 -eq 1 ]; then
    postinstall
else
    echo "This is an upgrade"
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
