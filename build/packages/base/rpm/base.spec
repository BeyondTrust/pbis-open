# ex: set tabstop=4 expandtab shiftwidth=4:
%if %{AIX}
%define _prefix /opt
%define __install /usr/linux/bin/install
%endif

%define _sysconfdir /etc

%{!?AIX: %define AIX 0}
%{!?Compat32: %define Compat32 0}

Name: 		%{RpmName}
Summary: 	Likewise Utility Libraries
Version: 	%{RpmVersion}
Release: 	%{RpmRelease}
License: 	LGPLv2.1
URL: 		http://www.likewise.com/
Group: 		Development/Libraries
BuildRoot: 	%{buildRootDir}/%{name}-%{version}
%if ! %{AIX}
Prereq: grep, sh-utils
%endif

%if %{AIX}
AutoReqProv: no
AutoReq: no
AutoProv: no
%endif

%if %{AIX}
%define INIT_DIR /etc/rc.d/init.d
%else
%define INIT_DIR /etc/init.d
%endif

%description
The Likewise Utility Libraries provide basic system management interface
for the Likewise domain join and domain integration services.

%package devel
Summary: Likewise Utility Software Development Kit
Group: Development/Libraries
Requires: likewise-base

%description devel
The likewise-base-devel package includes the development libraries and header files that supply the application programming interface for various utilities including string manipulation, unicode string support, threading, security etc..

%prep

%build

%install
rsync -a %{PopulateRoot}/ ${RPM_BUILD_ROOT}/


%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%if %{Compat32}
%define _lib lib
%endif

%files 
%defattr(-,root,root)

%{PrefixDir}/%{_lib}/*
%if !%{Compat32}
/etc/init.d/*
%{PrefixDir}/bin/*
%{PrefixDir}/sbin/*
%endif

%files devel
%defattr(0644,root,root,0755)

%attr(0644,root,root) %{PrefixDir}/%{_lib}/*.a
%attr(0644,root,root) %{PrefixDir}/%{_lib}/*.la
%if ! %{Compat32}
%attr(0644,root,root) %{PrefixDir}/include/*
%attr(0644,root,root) %{PrefixDir}/share/man/*
%attr(0644,root,root) %{PrefixDir}/share/doc/*
%endif

%if ! %{Compat32}
%define initScriptPathList %{INIT_DIR}/lwsmd
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
