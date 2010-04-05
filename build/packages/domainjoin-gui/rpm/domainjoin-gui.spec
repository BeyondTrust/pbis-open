%if %{AIX}
	%define __install /usr/linux/bin/install
%endif

%{!?i386compat: %define i386compat 0}
%{!?AIX: %define AIX 0}

%define _sysconfdir	/etc

Name: 		%{RpmName}
Summary: 	Graphical version of the Likewise Domain Join utility
Version: 	%{RpmVersion}
Release: 	%{RpmRelease}
License:	GPL
Group: 		System Environment/Daemons
URL: 		http://www.likewise.com/
BuildRoot:      %{buildRootDir}/%{name}-%{version}
Requires:	libglade2, likewise-domainjoin

AutoReq: no
%if %{AIX}
AutoReqProv: no
AutoProv: no
%endif

%description 
The Likewise graphical domain join tool provides an easy means of 
joining Linux clients to an Active Directory domain from supported 
desktop environments.

%prep

%build

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
rsync -a %{PopulateRoot}/ ${RPM_BUILD_ROOT}/

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root)
%{PrefixDir}/share/likewise/*
%{PrefixDir}/bin/domainjoin-gui
/usr/bin/domainjoin-gui


%changelog
