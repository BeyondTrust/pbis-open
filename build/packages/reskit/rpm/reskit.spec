# ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4:

%if %{AIX}
    %define __install /usr/linux/bin/install
%endif

%{!?Compat32: %define Compat32 0}
%{!?AIX: %define AIX 0}

Name: 		%{RpmName}
Summary:	Likewise Resources
Version: 	%{RpmVersion}
Release: 	%{RpmRelease}
License: 	Likewise Proprietary
Group: 		Development/Libraries
URL: 		http://www.likewise.com/
BuildRoot: 	%{buildRootDir}/%{name}-%{version}

AutoReq: no
%if %{AIX}
AutoReqProv: no
AutoProv: no
%endif

%description
The Likewise Resources package provides product documentation and tools used for gathering deployment diagnostics information.

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
%{PrefixDir}/reskit/*
%{PrefixDir}/docs/*
%endif


%changelog
