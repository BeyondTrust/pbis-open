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
Summary:	Likewise Test Framework
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
The Likewise Test Framework

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
%{PrefixDir}/bin/*
%endif

%changelog
