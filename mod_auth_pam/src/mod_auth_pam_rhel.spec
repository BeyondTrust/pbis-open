Summary: Installs mod_auth_pam.so and mod_auth_sys_group.so on RHEL
Name: mod_auth_pam_rhel
Version: 1.0
Release: 1
Group: Apache-Modules
Copyright: Commercial
Source: %{name}-%{version}.tar.bz2
Requires: httpd, pam
%description

%prep
%setup
%build

%install
cp mod_auth_pam.so /usr/lib/httpd/modules/mod_auth_pam.so
cp mod_auth_sys_group.so /usr/lib/httpd/modules/mod_auth_sys_group.so

%post

%clean

%files
%defattr(-,root,root)
/usr/lib/httpd/modules/mod_auth_pam.so
/usr/lib/httpd/modules/mod_auth_sys_group.so
