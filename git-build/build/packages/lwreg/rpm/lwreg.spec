# ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4:

%{!?i386compat: %define i386compat 0}

Name: 		__PKG_NAME
Summary:	Likewise Registry Service
Version: 	__PKG_VERSION
Release: 	1
License: 	GPLv2+
Group: 		Development/Libraries
URL: 		http://www.likewise.com/
BuildRoot: 	/var/tmp/%{name}-%{version}

Requires: likewise-open-libs, likewise-open-rpc, likewise-open-lwio


AutoReq: no

%define INIT_DIR /etc/init.d
%define _LIB lib

%ifarch x86_64
%define _LIB lib64
%endif

%if %{i386compat}
%define _LIB lib
%endif

%description
Likewise Server Service

%prep

%build

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
rsync -a __PKG_POPULATE_DIR/ ${RPM_BUILD_ROOT}/


%clean 
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{INIT_DIR}/*
%config /etc/likewise/*.reg
%{PrefixDir}/sbin/*
%{PrefixDir}/bin/*

%{PrefixDir}/%{_LIB}/*

%define initScriptPathList %{INIT_DIR}/lwregd
%post
## chkconfig behaves differently on various updates of RHEL and SUSE
## So, we massage the init script according to the release, for now.
for daemon in %{initScriptPathList}; do
    if [ -x $daemon ]; then
        if grep "LWI_STARTUP_TYPE_" $daemon >/dev/null 2>&1; then
            daemon_new=${daemon}.new

            if [ -f /etc/redhat-release ]; then
                /bin/sed \
                    -e 's/^#LWI_STARTUP_TYPE_REDHAT\(.*\)$/\1/' \
                    -e'/^#LWI_STARTUP_TYPE_SUSE.*$/ d' \
                    $daemon > $daemon_new
            else
                /bin/sed \
                    -e 's/^#LWI_STARTUP_TYPE_SUSE\(.*\)$/\1/' \
                    -e '/^#LWI_STARTUP_TYPE_REDHAT.*$/ d' \
                    $daemon > $daemon_new
            fi
            mv $daemon_new $daemon
            chmod 0755 $daemon
        fi
    fi
done

%changelog
