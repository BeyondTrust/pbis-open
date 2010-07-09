# ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4:

%if %{AIX}
    %define __install /usr/linux/bin/install
%endif

%define _sysconfdir	/etc

%{!?i386compat: %define i386compat 0}
%{!?AIX: %define AIX 0}

%if %{Compat32}
%define _lib lib
%endif

Name: 		%{RpmName}
Summary:	Likewise Security and Authentication Subsystem (LSASS)
Version: 	%{RpmVersion}
Release: 	%{RpmRelease}
License: 	Likewise Proprietary
Group: 		Development/Libraries
URL: 		http://www.likewise.com/
BuildRoot: 	%{buildRootDir}/%{name}-%{version}

Requires: likewise-rpc, likewise-krb5, likewise-openldap

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

%define _LIB lib

%ifarch x86_64
%define _LIB lib64
%endif

%if %{i386compat}
%define _LIB lib
%endif


%description
The Likewise Security and Authentication Subsystem
contains components to authenticate users and groups
in Windows Active Directory from Linux/UNIX hosts.

%package devel
Summary: Likewise Security and Authentication Subsystem Software Development Kit
Group: Development/Libraries
Requires: likewise-lsass

%description devel
The likewise-lsass-devel package includes the development libraries and header files that supply the application programming interface for the security and authentication subsystem (LSASS), which is used to authenticate users and groups in Windows Active Directory from Linux/UNIX hosts.

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
%{INIT_DIR}/*
%config(noreplace) /etc/likewise/gss/mech
%{PrefixDir}/sbin/*
%{PrefixDir}/bin/*
%{PrefixDir}/share/config/*
%endif
%{PrefixDir}/%{_lib}/*.so
%{PrefixDir}/%{_lib}/*.so.*
%{PrefixDir}/%{_lib}/gss/*.so
/%{_lib}/*.so
/%{_lib}/*.so.*
/%{_lib}/security/*.so

%if ! %{Compat32}
%define initScriptPathList %{INIT_DIR}/lsassd
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

%attr(0644,root,root) %{PrefixDir}/%{_lib}/*.a
%attr(0644,root,root) %{PrefixDir}/%{_lib}/*.la
%{PrefixDir}/%{_lib}/gss/*.a
%{PrefixDir}/%{_lib}/gss/*.la
/%{_lib}/*.a
/%{_lib}/*.la
/%{_lib}/security/*.la
%if ! %{Compat32}
%attr(0644,root,root) %{PrefixDir}/include/*
%attr(0644,root,root) %{PrefixDir}/share/doc/*
#%attr(0644,root,root) %{PrefixDir}/share/man/*
%endif

%changelog
