# ex: set tabstop=4 expandtab shiftwidth=4:
%if %{AIX}
%define __install /usr/linux/bin/install
%endif

%define _sysconfdir /etc

%{!?AIX: %define AIX 0}
%{!?i386compat: %define i386compat 0}

Name: 		%{RpmName}
Summary: 	Likewise Passwd Utility
Version: 	%{RpmVersion}
Release: 	%{RpmRelease}
License: 	GPL
URL: 		http://www.likewise.com/
Group: 		System
BuildRoot: 	%{buildRootDir}/%{name}-%{version}

%if %{AIX}
AutoReqProv: no
AutoReq: no
AutoProv: no
%endif

%description
The Likewise Passwd utility is a replacement for the standard
/usr/bin/passwd tool on systems with a non-PAM enabled passwd
change tool.

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

%changelog
