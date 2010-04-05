# ex: set tabstop=4 expandtab shiftwidth=4:

%{!?Compat32: %define Compat32 0}

Name: 		__PKG_NAME
Summary: 	Likewise Utility Libraries
Version: 	__PKG_VERSION
Release: 	1
License: 	Various (BSD & LGPLv2.1)
URL: 		http://www.likewisesoftware.com/
Group: 		Development/Libraries
BuildRoot: 	/var/tmp/%{name}-%{version}

Prereq: grep, sh-utils


%description
The Likewise Utility Libraries provide basic system management interface
for the Likewise domain join and domain integration services.

%prep

%build

%install
rsync -a __PKG_POPULATE_DIR/ ${RPM_BUILD_ROOT}/


%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%if %{Compat32}
%define _lib lib
%endif

%files 
%defattr(-,root,root)

%{_sysconfdir}/likewise/krb5.conf
%{_sysconfdir}/krb5.conf.default
%dir /var/lib/likewise/run

%{PrefixDir}/%{_lib}/*
%if !%{Compat32}
%{PrefixDir}/bin/*
%endif

%changelog
