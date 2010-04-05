%if %{AIX}
%define __install /usr/linux/bin/install
%endif

%{!?i386compat: %define i386compat 0}
%{!?AIX: %define AIX 0}

%define _sysconfdir	/etc

Name: 		%{RpmName}
Summary: 	LibXML2 libraries and development files
Version:	%{RpmVersion}
Release:	%{RpmRelease}
License: 	MIT License
Group: 		Development/Libraries
URL: 		http://xmlsoft.org
BuildRoot: 	%{buildRootDir}/%{name}-%{version}
Obsoletes: likewise-expat <= 2.0.0

%if %{AIX}
AutoReqProv: no
AutoReq: no
AutoProv: no
%endif


%description
LIbXML2 is an XML parser written in C. It aims to be fully comforming.

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

%changelog
