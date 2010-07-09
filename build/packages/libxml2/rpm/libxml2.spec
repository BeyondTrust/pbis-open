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

%package devel
Summary: LibXML2 libraries and development files
Group: Development/Libraries
Requires: likewise-libxml2

%description devel
The likewise-libxml2-devel package includes the development libraries and header files that supply the application programming interface for an LibXML2 parser written in C.
 
%prep

%build

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
rsync -a %{PopulateRoot}/ ${RPM_BUILD_ROOT}/

%clean 
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root)
%{PrefixDir}/%{_lib}/*.so
%{PrefixDir}/%{_lib}/*.so.*


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
