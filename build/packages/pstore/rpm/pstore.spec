# ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4:

%if %{AIX}
    %define __install /usr/linux/bin/install
%endif

%define _sysconfdir	/etc

%{!?i386compat: %define i386compat 0}
%{!?AIX: %define AIX 0}

Name: 		%{RpmName}
Summary:	Likewise Password Store
Version: 	%{RpmVersion}
Release: 	%{RpmRelease}
License: 	Likewise Proprietary
Group: 		Development/Libraries
URL: 		http://www.likewise.com/
BuildRoot: 	%{buildRootDir}/%{name}-%{version}

Requires: likewise-sqlite

AutoReq: no
%if %{AIX}
AutoReqProv: no
AutoProv: no
%endif

%description
The Likewise Password Storage maintains domain information
and the machine password from Active Directory.

%prep

%build

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
rsync -a %{PopulateRoot}/ ${RPM_BUILD_ROOT}/

%clean 
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%config(noreplace) /etc/likewise/pstore.reg
%{PrefixDir}/%{_lib}/*
%{PrefixDir}/bin/*

%changelog
