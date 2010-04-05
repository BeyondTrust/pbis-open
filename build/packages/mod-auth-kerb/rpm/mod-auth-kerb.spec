%if %{AIX}
%define _prefix /opt
%define __install /usr/linux/bin/install
%endif

%{!?i386compat: %define i386compat 0}
%{!?AIX: %define AIX 0}

Name: 		%{RpmName}
Summary: Likewise Apache Kerberos Auth
Version: 	%{RpmVersion}
Release: 	%{RpmRelease}
License: 	BSD License
Group: Development/Libraries
URL: http://modauthkerb.sourceforge.net/
BuildRoot: %{buildRootDir}/%{name}-%{version}

Requires: likewise-krb5

AutoReq: no
%if %{AIX}
AutoReqProv: no
AutoProv: no
%endif

%description
A Kerberos module for Apache to allow AD authentication of users


%prep


%build

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
rsync -a %{PopulateRoot}/ ${RPM_BUILD_ROOT}/


%clean 
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{PrefixDir}/apache/*

%changelog
