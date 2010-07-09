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

Summary: 	Kerberos 5 Client libraries
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
Requires: likewise-krb5 = %{version}
%endif

%if %{AIX}
AutoReqProv: no
AutoReq: no
AutoProv: no
%endif

%description
Kerberos is a network authentication system. The likewise-krb5 package
contains the header files and libraries needed for compiling Kerberos
5 programs. If you want to develop Kerberos-aware programs, you need
to install this package.

%package devel
Summary: The likewise-krb5-devel package contains the header files and libraries needed for compiling Kerberos 5 programs.
Group: Development/Libraries
Requires: likewise-krb5
%description devel
Kerberos is a network authentication system. The likewise-krb5-devel package
contains the header files and libraries needed for compiling Kerberos 5
programs.

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

%{PrefixDir}/%{_lib}/*.so
%{PrefixDir}/%{_lib}/*.so.*
%{PrefixDir}/%{_lib}/krb5/plugins/preauth/*.so

%files devel
%defattr(0644,root,root,0755)

#%attr(0644,root,root) %{PrefixDir}/%{_lib}/*.a
#%attr(0644,root,root) %{PrefixDir}/%{_lib}/*.la
%attr(0644,root,root) %{PrefixDir}/include/*
#%attr(0644,root,root) %{PrefixDir}/man/*

%changelog
