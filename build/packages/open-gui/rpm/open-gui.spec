%{!?AIX: %define AIX 0}

%{!?Compat32: %define Compat32 0}

%if %{AIX}
%define _prefix /opt
%define __install /usr/linux/bin/install
%endif

%define _sysconfdir /etc

%if %{Compat32}
%define _lib lib
%endif

%define _LIB lib
%ifarch x86_64
%define _LIB lib64
%endif

Name: 		%{RpmName}
Summary: 	Likewise Open is amazing
Version: 	%{RpmVersion}
Release: 	%{RpmRelease}
License: 	Likewise Proprietary
URL: 		http://www.likewise.com/
Group: 		System Environment/Daemons
BuildRoot: 	%{buildRootDir}/%{name}-%{version}
%if ! %{AIX}
Prereq: grep, sh-utils
%endif

AutoReq: no
%if %{AIX}
AutoReqProv: no
AutoProv: no
%endif

%description
Likewise provides Active Directory authentication.

%prep

%build

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
rsync -a %{PopulateRoot}/ ${RPM_BUILD_ROOT}/

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root)
%if !%{Compat32}
%{PrefixDir}/bin/domainjoin-gui
%{PrefixDir}/share/likewise/domainjoin-logo.png
%{PrefixDir}/share/likewise/likewise-logo.png
%{PrefixDir}/share/likewise/domainjoin-gtk.glade
/usr/bin/domainjoin-gui
%endif

%changelog
