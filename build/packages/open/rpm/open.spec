# ex: set tabstop=4 expandtab shiftwidth=4:

%{!?AIX: %define AIX 0}

%{!?Compat32: %define Compat32 0}

%if %{AIX}
%define _prefix /opt
%define __install /usr/linux/bin/install
%define INIT_DIR /etc/rc.d/init.d
%else
%define INIT_DIR /etc/init.d
%endif

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
%if ! %{AIX}
Prereq: grep, sh-utils
%endif

AutoReq: no
%if %{AIX}
AutoReqProv: no
AutoProv: no
%endif


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
%{PrefixDir}/%{_lib}/gss/*.so
#%{PrefixDir}/%{_lib}/security/*.so
%{PrefixDir}/%{_lib}/engines/*.so
%{PrefixDir}/%{_lib}/lwsm-loader/*.so
%{PrefixDir}/%{_lib}/krb5/plugins/preauth/*.so
%{PrefixDir}/%{_lib}/sasl2/lib*
%{PrefixDir}/ssl
%endif

%if ! %{Compat32}
%define initScriptPathList %{INIT_DIR}/dcerpcd %{INIT_DIR}/eventlogd %{INIT_DIR}/lsassd %{INIT_DIR}/lwiod %{INIT_DIR}/lwregd %{INIT_DIR}/lwsmd %{INIT_DIR}/netlogond
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

DOMAINJOIN=/opt/likewise/bin/domainjoin-cli
LWSM=/opt/likewise/bin/lwsm
LWSMD=/etc/init.d/lwsmd
REGSHELL=/opt/likewise/bin/lwregshell

$LWSMD start || exit 4

$REGSHELL import /opt/likewise/share/config/dcerpcd.reg || exit 100
$REGSHELL import /opt/likewise/share/config/eventlogd.reg || exit 101
$REGSHELL import /opt/likewise/share/config/lwreg.reg || exit 102
$REGSHELL import /opt/likewise/share/config/lsassd.reg || exit 103
$REGSHELL import /opt/likewise/share/config/lwiod.reg || exit 104
$REGSHELL import /opt/likewise/share/config/netlogond.reg || exit 105
$REGSHELL import /opt/likewise/share/config/pstore.reg || exit 106

$LWSMD reload || exit 5

$DOMAINJOIN query > /dev/null 2>&1 || exit 6

%endif

%files devel
%defattr(0644,root,root,0755)

%attr(0644,root,root) %{PrefixDir}/%{_lib}/*.a
%attr(0644,root,root) %{PrefixDir}/%{_lib}/*.la
/%{_lib}/*.a
/%{_lib}/*.la
/%{_lib}/security/*.la

%if ! %{Compat32}
%attr(0644,root,root) %{PrefixDir}/include/*
%attr(0644,root,root) %{PrefixDir}/share/man/*
%attr(0644,root,root) %{PrefixDir}/share/doc/*
%attr(0644,root,root) %{PrefixDir}/ssl/*
%attr(0644,root,root) %{PrefixDir}/%{_lib}/lwsm-loader/*.la
%attr(0644,root,root) %{PrefixDir}/%{_lib}/pkgconfig/*
%{PrefixDir}/%{_lib}/gss/*.a
%{PrefixDir}/%{_lib}/gss/*.la
%endif

%changelog
