# ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4:

%{!?Compat32: %define Compat32 0}


Summary:	Likewise Audit (Eventlog)
Version: 	%{RpmVersion}
Release: 	%{RpmRelease}
License: 	Likewise Proprietary
Group: 		Development/Libraries
URL: 		http://www.likewise.com/
BuildRoot: 	%{buildRootDir}/%{name}-%{version}

Requires: likewise-rpc, likewise-krb5, likewise-openldap


Name: 		%{RpmName}

AutoReq: no

%if %{Compat32}
%define _lib lib
%endif


%description
Likewise Audit (Eventlog) system

%package devel
Summary: Likewise Audit (Eventlog) Software Development Kit
Group: Development/Libraries
Requires: likewise-eventlog

%description devel
The likewise-eventlog-devel package includes the development libraries and header files that supply the application programming interface for the Likewise Audit (Eventlog) system.

%prep

%build

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
rsync -a %{PopulateRoot}/ ${RPM_BUILD_ROOT}/


%clean 
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{PrefixDir}/sbin/*
%{PrefixDir}/bin/*
%{PrefixDir}/%{_lib}/*.so
%{PrefixDir}/%{_lib}/*.so.*
%{PrefixDir}/share/config/*
/var/lib/likewise/lwreport.xml

%post
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
