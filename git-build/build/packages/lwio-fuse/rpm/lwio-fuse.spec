# ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4:

%{!?Compat32: %define Compat32 0}

Name: 		__PKG_NAME
Summary:	Likewise SMB FUSE mount utility
Version: 	__PKG_VERSION
Release: 	1
License: 	GPLv2+
Group: 		Development/Libraries
URL: 		http://www.likewise.com/
BuildRoot: 	/var/tmp/%{name}-%{version}

Requires: likewise-open-libs likewise-open-netlogon


AutoReq: no

%define INIT_DIR /etc/init.d

%if %{Compat32}
%define _lib lib
%endif

%description
Likewise SMB FUSE mount utility

%prep

%build

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
rsync -a __PKG_POPULATE_DIR/ ${RPM_BUILD_ROOT}/


%clean 
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{PrefixDir}/bin/*


%changelog
