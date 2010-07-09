# ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4:

%define _sysconfdir	/etc

%{!?Compat32: %define Compat32 0}


Summary:	Likewise Management Services
Version: 	%{RpmVersion}
Release: 	%{RpmRelease}
License: 	Likewise Proprietary
Group: 		Development/Libraries
URL: 		http://www.likewise.com/
BuildRoot: 	%{buildRootDir}/%{name}-%{version}

Requires: likewise-rpc, likewise-krb5, likewise-lsass

Name: 		%{RpmName}

AutoReq: no

%define INIT_DIR /etc/init.d

%if %{Compat32}
%define _lib lib
%endif

%description
Likewise Management System

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
%config(noreplace) /etc/likewise/lwmgmtd.conf
%{PrefixDir}/sbin/*
%{PrefixDir}/bin/*
%{PrefixDir}/%{_lib}/*.so
%{PrefixDir}/%{_lib}/*.so.*

%define initScriptPathList %{INIT_DIR}/lwmgmtd
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


%changelog
