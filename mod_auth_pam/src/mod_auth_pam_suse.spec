Summary: Installs mod_auth_pam.so and mod_auth_sys_group.so on SUSE9.x
Name: mod_auth_pam_suse
Version: 1.0
Release: 1
Group: Apache-Modules
Copyright: Commercial
Source: %{name}-%{version}.tar.bz2
Requires: apache2, pam
%description

%prep
%setup

%build

%install
cp mod_auth_pam.so /usr/lib/apache2/mod_auth_pam.so
cp mod_auth_sys_group.so /usr/lib/apache2/mod_auth_sys_group.so

%post

%clean

%files
%defattr(-,root,root)
/usr/lib/apache2/mod_auth_pam.so
/usr/lib/apache2/mod_auth_sys_group.so
