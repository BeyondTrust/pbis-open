Summary: Installs mod_auth_pam.so and mod_auth_sys_group.so on SUSE9.x
Name: mod_auth_pam
Version: 1.1.1
Release: 1
Group: Apache-Modules
Copyright: Commercial
Source: %{name}-%{version}.tar.bz2
BuildRequires: apache2 pam
Requires: apache2 pam
Obsoletes: mod_auth_pam mod_auth_pam_suse mod_auth_pam_rhel
Provides: mod_auth_pam mod_auth_pam_suse mod_auth_pam_rhel

%description

%prep
%setup

%build
  make
#  cp %{_libdir}/apache2/mod_auth_pam.so .
#  cp %{_libdir}/apache2/mod_auth_sys_group.so .
%install
  make install
#cp mod_auth_pam.so %{_libdir}/apache2/mod_auth_pam.so
#cp mod_auth_sys_group.so %{_libdir}/apache2/mod_auth_sys_group.so

%post

%clean
  make clean
  rm -rf %{buildroot}
%files
%defattr(-,root,root)
%{_libdir}/apache2/mod_auth_pam.so
%{_libdir}/apache2/mod_auth_sys_group.so
