# ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4:

%if %{AIX}
    %define __install /usr/linux/bin/install
%endif

%define _sysconfdir	/etc

%{!?Compat32: %define Compat32 0}
%{!?AIX: %define AIX 0}

%if %{Compat32}
%define _lib lib
%endif

Name: 		%{RpmName}
Summary:	Likewise Group Policy Agent
Version: 	%{RpmVersion}
Release: 	%{RpmRelease}
License: 	Likewise Proprietary
Group: 		Development/Libraries
URL: 		http://www.likewise.com/
BuildRoot: 	%{buildRootDir}/%{name}-%{version}

Requires: likewise-krb5, likewise-openldap, likewise-libxml2

AutoReq: no
%if %{AIX}
AutoReqProv: no
AutoProv: no
%endif

%if %{AIX}
%define INIT_DIR /etc/rc.d/init.d
%else
%define INIT_DIR /etc/init.d
%endif


%description
The Likewise Group Policy Agent contains a Windows Active
Directory compatible group policy daemon which allows 
centralized policy deployment to Linux and Unix hosts.

%prep

%build

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
rsync -a %{PopulateRoot}/ ${RPM_BUILD_ROOT}/


%clean 
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{PrefixDir}/%{_lib}/*
%if ! %{Compat32}
%{INIT_DIR}/*
%config(noreplace) /etc/likewise/grouppolicy.reg
%config(noreplace) /etc/likewise/gpagentd.reg
/etc/likewise/*.reg
%{PrefixDir}/sbin/*
%{PrefixDir}/bin/*
%endif

%if ! %{Compat32}
%define initScriptPathList %{INIT_DIR}/gpagentd
%post
## chkconfig behaves differently on various updates of RHEL and SUSE
## So, we massage the init script according to the release, for now.
for daemon in %{initScriptPathList}; do
    if [ -x $daemon ]; then
        if grep "LWI_STARTUP_TYPE_" $daemon >/dev/null 2>&1; then
            daemon_new=${daemon}.new

            if [ -f /etc/redhat-release ]; then
                base_pri=19
                if [ -f /etc/rc2.d/S*NetworkManager ]; then
                    base_pri=$(( `basename /etc/rc2.d/S*NetworkManager | sed 's/S\([0-9]\+\).*/\1/' ` + 1 ))
                fi
                start_pri=`/bin/sed -n -e 's/.* chkconfig: [^ ]\+ \([0-9]\+\) [0-9]\+.*$/\1/p' $daemon`
                /bin/sed \
                    -e 's/^#LWI_STARTUP_TYPE_REDHAT\(.*\)$/\1/' \
                    -e '/^#LWI_STARTUP_TYPE_SUSE.*$/ d' \
                    -e 's/\(.* chkconfig: [^ ]\+ \)\([0-9]\+\)\( [0-9]\+.*\)$/\1'$(( $start_pri - 19 + $base_pri ))'\3/' \
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
%endif

%changelog
