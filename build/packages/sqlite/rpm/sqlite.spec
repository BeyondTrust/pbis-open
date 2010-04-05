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

Summary: 	SQLite Client libraries
Version: 	%{RpmVersion}
Release: 	%{RpmRelease}
License: 	MIT, freely distributable.
URL: 		http://www.sqlite.org
Group: 		Development/Libraries
BuildRoot: 	%{buildRootDir}/%{name}-%{version}
%if ! %{AIX}
Prereq: grep, sh-utils
%endif
%if %{i386compat}
Requires: likewise-sqlite = %{version}
%endif

%if %{AIX}
AutoReqProv: no
AutoReq: no
AutoProv: no
%endif

%description
Sqlite is a SQL database engine.

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

%{PrefixDir}/%{_lib}/*

%changelog
