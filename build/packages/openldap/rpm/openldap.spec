%if %{AIX}
%define _prefix /opt
%define __install /usr/linux/bin/install
%endif

%{!?i386compat: %define i386compat 0}
%{!?AIX: %define AIX 0}

%if ! %{i386compat}
Name: 		%{RpmName}
%else
Name: 		%{RpmName}-32bit
%define _lib lib
%endif

Summary: OpenLDAP client libraries and development files
Version: 	%{RpmVersion}
Release: 	%{RpmRelease}
License: 	OpenLDAP Public License
Group: Development/Libraries
URL: http://www.openldap.org/
BuildRoot: %{buildRootDir}/%{name}-%{version}
%if %{i386compat}
Requires: likewise-openldap = %{version}, likewise-krb5
%endif

%if %{AIX}
AutoReqProv: no
AutoReq: no
AutoProv: no
%endif

%description
The openldap package includes the runtime libraries needed to use LDAP
(Lightweight Directory Access Protocol) internals. LDAP is a set of
protocols for enabling directory services over the Internet.

%package devel
Summary: The likewise-openldap-devel package includes the development libraries and header files needed for compiling applications that use LDAP (Lightweight Directory Access Protocol) internals. LDAP is a set of protocols for enabling directory services over the Internet. Install this package only if you plan to develop or will need to compile customized LDAP clients.
Group: Development/Libraries
Requires: likewise-openldap
%description devel
The likewise-openldap-devel package contains the header files and libraries needed to develop software that utilizes the Lightweight Directory Access Protocol (LDAP).

%prep


%build

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
rsync -a %{PopulateRoot}/ ${RPM_BUILD_ROOT}/


%clean 
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{PrefixDir}/%{_lib}/libldap*.so
%{PrefixDir}/%{_lib}/libldap*.so.*
%{PrefixDir}/%{_lib}/liblber*.so
%{PrefixDir}/%{_lib}/liblber*.so.*
%{PrefixDir}/%{_lib}/libsasl*
%{PrefixDir}/%{_lib}/sasl2/lib*
%{PrefixDir}/bin/openldap/*

%files devel
%defattr(0644,root,root,0755)

#%attr(0644,root,root) %{PrefixDir}/%{_lib}/*.a
%attr(0644,root,root) %{PrefixDir}/%{_lib}/*.la
#%attr(0644,root,root) %{PrefixDir}/%{_lib}/sasl2/*.a
#%attr(0644,root,root) %{PrefixDir}/%{_lib}/sasl2/*.la
%attr(0644,root,root) %{PrefixDir}/include/*
#%attr(0644,root,root) %{PrefixDir}/man/*


%changelog
