%if %{AIX}
	%define __install /usr/linux/bin/install
%endif

%{!?i386compat: %define i386compat 0}
%{!?AIX: %define AIX 0}

%define _sysconfdir	/etc

Name: 		%{RpmName}
Summary: 	Likewise Domain Join (CLI)
Version: 	%{RpmVersion}
Release: 	%{RpmRelease}
License:	GPL/LGPL
Group: 		System Environment/Daemons
URL: 		http://www.likewise.com/
BuildRoot:      %{buildRootDir}/%{name}-%{version}

AutoReq: no
%if %{AIX}
AutoReqProv: no
AutoProv: no
%endif

%description
The Likewise Domain Join Utiltities provides the means to join Unix Systems
to Windows Active Directory Domains.

%prep

%build

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
rsync -a %{PopulateRoot}/ ${RPM_BUILD_ROOT}/
rm ${RPM_BUILD_ROOT}/%{PrefixDir}/%{_lib}/*.la

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT


%files 
%defattr(-,root,root)
%{PrefixDir}/%{_lib}/*.so*
%{PrefixDir}/bin/ConfigureLogin
%{PrefixDir}/bin/gpcron
%{PrefixDir}/bin/domainjoin-cli
/usr/bin/domainjoin-cli
%{PrefixDir}/data/VERSION


%changelog
