Name:       yast2-likewise
Summary:    YaST2 - Likewise Enterprise
Version:    %{AddonVer}
Release:    1
License:    LGPLv2.1
URL:        http://www.likewise.com/
Group:      Development/Libraries
BuildRoot:  %{_builddir}/%{name}-root

%description
This contains yast files necessary for the Likewise configuration workflow on SuSE.

%prep

%build

%install
cp -p -r %{rpmStaging}/* ${RPM_BUILD_ROOT}/

%clean
:

%files
%defattr(-,root,root)

/usr/share/YaST2/modules/*
/usr/share/YaST2/clients/*
/etc/likewise/*

%changelog
