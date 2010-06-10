# ex: set tabstop=4 expandtab shiftwidth=4:
%if %{AIX}
%define __install /usr/linux/bin/install
%endif

%if %{AIX}
%define INIT_DIR /etc/rc.d/init.d
%else
%define INIT_DIR /etc/init.d
%endif

%define _sysconfdir /etc

%{!?AIX: %define AIX 0}
%{!?i386compat: %define i386compat 0}

Name:		%{RpmName}
Summary: 	DCE/RPC Runtime and APIs
Version: 	%{RpmVersion}
Release: 	%{RpmRelease}
License: 	Likewise
URL: 		http://www.likewise.com/
Group: 		Development/Libraries
BuildRoot: 	%{buildRootDir}/%{name}-%{version}

%if %{AIX}
AutoReqProv: no
AutoReq: no
AutoProv: no
%endif

%description
The DCE/RPC runtime environment is used to provide access to management 
services remogtely across a network.  These libraries support features
such as user management and Active Directory integration.

%package devel
Summary: Likewise RPC Software Development Kit
Group: Development/Libraries
Requires: likewise-rpc

%description devel
The likewise-rpc-devel package includes the development libraries and header files that supply the application programming interface for the DCE/RPC runtime.

%prep

%build

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
rsync -a %{PopulateRoot}/ ${RPM_BUILD_ROOT}/


%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files 
%defattr(-,root,root)
%{PrefixDir}/%{_lib}/*.so
%{PrefixDir}/%{_lib}/*.so.*
%{_sysconfdir}/init.d/*
%dir /var/rpc

%if ! %{Compat32}
%{PrefixDir}/bin/rpcping
%{PrefixDir}/sbin
%{PrefixDir}/share/config/*
%{PrefixDir}/share/dce-rpc/*
%define initScriptPathList %{INIT_DIR}/dcerpcd
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


%files devel
%defattr(0644,root,root,0755)

%attr(0755,root,root) %{PrefixDir}/bin/uuid
%attr(0755,root,root) %{PrefixDir}/bin/dceidl
%attr(0644,root,root) %{PrefixDir}/%{_lib}/*.a
%attr(0644,root,root) %{PrefixDir}/%{_lib}/*.la
%if ! %{Compat32}
%attr(0644,root,root) %{PrefixDir}/include/*
#%attr(0644,root,root) %{PrefixDir}/share/man/*
#%attr(0644,root,root) %{PrefixDir}/share/doc/*
%endif

%changelog
