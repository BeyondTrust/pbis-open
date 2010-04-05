# ex: set tabstop=4 expandtab shiftwidth=4:
%if %{AIX}
%define _prefix /opt
%define __install /usr/linux/bin/install
%endif

%define _sysconfdir /etc

%{!?AIX: %define AIX 0}
%{!?i386compat: %define i386compat 0}

%if ! %{i386compat}
Name: 		%{RpmName}
%else
Name: 		%{RpmName}-32bit
%define _lib lib
%endif

Summary: 	Likewise QA Tools
Version: 	%{RpmVersion}
Release: 	%{RpmRelease}
License: 	MIT, freely distributable.
URL: 		http://web.mit.edu/kerberos/www/
Group: 		Development/Libraries
BuildRoot: 	%{buildRootDir}/%{name}-%{version}
%if ! %{AIX}
Prereq: grep, sh-utils
%endif
%if %{i386compat}
Requires: likewise-lsass = %{version}
%endif

%if %{AIX}
AutoReqProv: no
AutoReq: no
AutoProv: no
%endif

%description
Likewise QA Tools

%prep

%build

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
rsync -a %{PopulateRoot}/ ${RPM_BUILD_ROOT}/


%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files 
%defattr(-,root,root)

%if ! %{i386compat}
%{PrefixDir}/bin/*
%endif

# qatools currently does not have any library files
#%{PrefixDir}/%{_lib}/*

%changelog
