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

%package devel
Summary: Likewise Password Store Software Development Kit
Group: Development/Libraries
Requires: likewise-pstore

%description devel
The likewise-pstore-devel package includes the development libraries and header files that supply the application programming interface for Password Storage.

%prep

%build

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
rsync -a %{PopulateRoot}/ ${RPM_BUILD_ROOT}/

%clean 
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{PrefixDir}/%{_lib}/*
%{PrefixDir}/bin/*
%{PrefixDir}/share/config/*

%files devel
%defattr(0644,root,root,0755)

%attr(0644,root,root) %{PrefixDir}/%{_lib}/*.a
%attr(0644,root,root) %{PrefixDir}/%{_lib}/*.la
%if ! %{Compat32}
%attr(0644,root,root) %{PrefixDir}/include/*
#%attr(0644,root,root) %{PrefixDir}/share/man/*
#%attr(0644,root,root) %{PrefixDir}/share/doc/*
%endif

%changelog
