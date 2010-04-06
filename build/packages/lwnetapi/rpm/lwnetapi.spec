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
Summary:	Likewise Net API
Version: 	%{RpmVersion}
Release: 	%{RpmRelease}
License: 	Likewise Proprietary
Group: 		Development/Libraries
URL: 		http://www.likewise.com/
BuildRoot: 	%{buildRootDir}/%{name}-%{version}

Requires: likewise-lsass

AutoReq: no
%if %{AIX}
AutoReqProv: no
AutoProv: no
%endif

%define _LIB lib

%ifarch x86_64
%define _LIB lib64
%endif

%if %{i386compat}
%define _LIB lib
%endif


%description
Likewise Net API

%package devel
Summary: Likewise Net Software Development Kit
Group: Development/Libraries
Requires: likewise-lwnetapi

%description devel
The likewise-lwnetapi-devel package includes the development libraries and header files that supply the application programming interface for the Likewise Net utilities.

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
