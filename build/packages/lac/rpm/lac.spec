# ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4:
%if %{AIX}
    %define __install /usr/linux/bin/install
%endif

%{!?i386compat: %define i386compat 0}
%{!?AIX: %define AIX 0}

%define _sysconfdir /etc

Name: 		%{RpmName}
Summary: 	Likewise Administration Console
Version: 	%{RpmVersion}
Release: 	%{RpmRelease}
License: 	Likewise
Group: 		Administration/Management
URL: 		http://www.likewise.com/
BuildRoot: %{buildRootDir}/%{name}-%{version}

%if %{AIX}
AutoReqProv: no
AutoReq: no
AutoProv: no
%endif

%description
The Likewise Administration Console provides a set of plug-ins 
for performing tasks such as managing Active Directory domain 
controllers and modifying users and groups on remote CIFS servers.


%prep

%build

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
rsync -a %{PopulateRoot}/ ${RPM_BUILD_ROOT}/

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT



%files
%defattr(-,root,root,-)
%{PrefixDir}/*

%changelog
