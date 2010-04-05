# ex: set tabstop=4 expandtab shiftwidth=4:

%define INIT_DIR /etc/init.d
%{!?Compat32: %define Compat32 0}

Name:		__PKG_NAME
Summary: 	DCE/RPC Runtime and APIs
Version: 	__PKG_VERSION
Release: 	1
License: 	Various (LGPL & BSD)
URL: 		http://www.likewise.com/
Group: 		Development/Libraries
BuildRoot: 	/var/tmp/%{name}-%{version}


%description
The DCE/RPC runtime environment is used to provide access to management 
services remogtely across a network.  These libraries support features
such as user management and Active Directory integration.

%prep

%build

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
rsync -a __PKG_POPULATE_DIR/ ${RPM_BUILD_ROOT}/


%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files 
%defattr(-,root,root)
%{PrefixDir}/*
%{_sysconfdir}/init.d/*
%config /etc/likewise/dcerpcd.reg
%dir /var/lib/likewise/rpc

%if ! %{Compat32}
%define initScriptPathList %{INIT_DIR}/dcerpcd
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
%endif

%changelog
