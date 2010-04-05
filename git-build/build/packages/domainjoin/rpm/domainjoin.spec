%{!?i386compat: %define i386compat 0}

Name: 		__PKG_NAME
Summary: 	Likewise Domain Join Utilities
Version: 	__PKG_VERSION
Release: 	1
License:	GPLv2+/LGPLv2.1+
Group: 		System Environment/Daemons
URL: 		http://www.likewise.com/
BuildRoot:      /var/tmp/%{name}-%{version}

AutoReq: no

%description
The Likewise Domain Join Utiltities provides the means to join Unix Systems
to Windows Active Directory Domains.

%prep

%build

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
rsync -a __PKG_POPULATE_DIR/ ${RPM_BUILD_ROOT}/

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT


%files 
%defattr(-,root,root)
%{PrefixDir}/%{_lib}/*.so*
%{PrefixDir}/bin/ConfigureLogin
%{PrefixDir}/bin/gpcron
%{PrefixDir}/bin/domainjoin-*
%{PrefixDir}/share/likewise/*
%{PrefixDir}/data/VERSION


%changelog
