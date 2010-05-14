# ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4:

%if %{AIX}
    %define __install /usr/linux/bin/install
%endif

%define _sysconfdir	/etc

%{!?i386compat: %define i386compat 0}
%{!?AIX: %define AIX 0}

Name: 		%{RpmName}
Summary:	Likewise Netlogon
Version: 	%{RpmVersion}
Release: 	%{RpmRelease}
License: 	Likewise Proprietary
Group: 		Development/Libraries
URL: 		http://www.likewise.com/
BuildRoot: 	%{buildRootDir}/%{name}-%{version}

Requires: likewise-rpc, likewise-openldap

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
The Netlogon service allows clients to retrieve
information about domain controllers in a given domain.
in Windows Active Directory from Linux/UNIX hosts.

%package devel
Summary: Likewise Netlogon Software Development Kit
Group: Development/Libraries
Requires: likewise-netlogon
%description devel
The likewise-netlogon-devel package includes the development libraries and header files that supply the application programming interface to query information about domain controllers in a given domain.

%prep

%build

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
rsync -a %{PopulateRoot}/ ${RPM_BUILD_ROOT}/


%clean 
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{INIT_DIR}/*
%config %{_sysconfdir}/likewise/likewise-krb5-ad.conf
%{PrefixDir}/sbin/*
%{PrefixDir}/bin/*
%{PrefixDir}/%{_lib}/*
%{PrefixDir}/share/config/*

%define initScriptPathList %{INIT_DIR}/netlogond
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


%files devel
%defattr(0644,root,root,0755)

%attr(0644,root,root) %{PrefixDir}/%{_lib}/*.a
%attr(0644,root,root) %{PrefixDir}/%{_lib}/*.la
%attr(0644,root,root) %{PrefixDir}/include/*
#%attr(0644,root,root) %{PrefixDir}/man/*

%changelog
