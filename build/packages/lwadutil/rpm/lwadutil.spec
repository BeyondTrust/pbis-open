# ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4:

%define _sysconfdir	/etc

%{!?Compat32: %define Compat32 0}

Name: 		%{RpmName}
Summary:	Likewise Admin tool
Version: 	%{RpmVersion}
Release: 	%{RpmRelease}
License: 	Likewise Proprietary
Group: 		Development/Libraries
URL: 		http://www.likewise.com/
BuildRoot: 	%{buildRootDir}/%{name}-%{version}

Requires: likewise-krb5 likewise-netlogon likewise-openldap likewise-grouppolicy


AutoReq: no

%if %{Compat32}
%define _lib lib
%endif

%description
Likewise Admin tool is a tool to list, download or upload gpsettings from or to AD

%prep

%build

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
rsync -a %{PopulateRoot}/ ${RPM_BUILD_ROOT}/


%clean 
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{PrefixDir}/bin/*
%{PrefixDir}/%{_lib}/*

%changelog
