# ex: set tabstop=4 expandtab shiftwidth=4:

Name: 		__PKG_NAME
Summary: 	Likewise CIFS Server
Version: 	__PKG_VERSION
Release: 	1
License: 	HP
URL: 		http://www.likewise.com/
Group: 		Development/Libraries
BuildRoot: 	/var/tmp/%{name}-%{version}

Prereq: grep, sh-utils
Obsoletes:   likewise-open-libs, likewise-open-lsass, likewise-open-netlogon, likewise-open-lwio, likewise-open-eventlog, likewise-open-rpc, likewise-open-lwsm, likewise-open-lwreg, likewise-open-srvsvc


%description
The Likewise-CIFS server is a complete SMB/CIFS server stack
including standalone modes and membership in an AD domain,
user authentication, and DCE/RPC services.

%prep

%build

%install
rsync -a __PKG_POPULATE_DIR/ ${RPM_BUILD_ROOT}/

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%post
case "$1" in
    1)
    ## New install
    %{PrefixDir}/bin/domainjoin-cli configure --enable pam
    %{PrefixDir}/bin/domainjoin-cli configure --enable nsswitch

    ## chkconfig behaves differently on various updates of RHEL and SUSE
    ## So, we massage the init script according to the release, for now.
    for d in lsassd lwsmd lwiod eventlogd dcerpcd netlogond lwregd srvsvcd; do
	daemon="%{_sysconfdir}/init.d/$d"
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

    for d in lsassd lwsmd lwiod eventlogd dcerpcd netlogond lwregd srvsvcd; do
        chkconfig --add ${d}
    done

    %{_sysconfdir}/init.d/lwsmd start
    echo -n "Waiting for lwreg startup."
    while( test -z "`/opt/likewise/bin/lwsm status lwreg | grep standalone:`" )
    do
        echo -n "."
        sleep 1
    done
    echo "ok"
    for file in %{_sysconfdir}/likewise/*reg; do
        echo "Installing settings from $file..."
        %{PrefixDir}/bin/lwregshell import $file
    done
    %{_sysconfdir}/init.d/lwsmd reload
    sleep 2
    %{PrefixDir}/bin/lwsm start srvsvc
    ;;

    2)
    ## Upgrade

    ## chkconfig behaves differently on various updates of RHEL and SUSE
    ## So, we massage the init script according to the release, for now.
    for d in lsassd lwsmd lwiod eventlogd dcerpcd netlogond lwregd srvsvcd; do
	daemon="%{_sysconfdir}/init.d/$d"
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

    for d in lsassd lwsmd lwiod eventlogd dcerpcd netlogond lwregd srvsvcd; do
        chkconfig --add ${d}
    done

    [ -z "`pidof lwsmd`" ] && %{_sysconfdir}/init.d/lwsmd start

    echo -n "Waiting for lwreg startup."
    while( test -z "`/opt/likewise/bin/lwsm status lwreg | grep standalone:`" )
    do
        echo -n "."
        sleep 1
    done
    echo "ok"

    for file in %{_sysconfdir}/likewise/*reg; do
        echo "Upgrading settings from $file..."
        %{PrefixDir}/bin/lwregshell upgrade $file
    done
    %{_sysconfdir}/init.d/lwsmd reload
    sleep 2
    %{PrefixDir}/bin/lwsm stop lwreg
    %{PrefixDir}/bin/lwsm start srvsvc
    ;;

esac

%preun
if [ "$1" = 0 ]; then
    ## Be paranoid about cleaning up
    %{PrefixDir}/bin/domainjoin-cli leave
    %{PrefixDir}/bin/domainjoin-cli configure --disable pam
    %{PrefixDir}/bin/domainjoin-cli configure --disable nsswitch

    %{PrefixDir}/bin/lwsm stop lwreg
    %{_sysconfdir}/init.d/lwsmd stop
fi


%files
%defattr(-,root,root)

%{_sysconfdir}/likewise/krb5.conf
%{_sysconfdir}/krb5.conf.default
%dir /var/lib/likewise/run
%dir /var/lib/likewise/rpc

%{_sysconfdir}/init.d/*
%config %{_sysconfdir}/likewise/likewise-krb5-ad.conf
%config %{_sysconfdir}/likewise/gss/mech
%config %{_sysconfdir}/likewise/*.reg

%{PrefixDir}/share/likewise/*
%{PrefixDir}/data/VERSION

/%{_lib}/*.so*
/%{_lib}/security/*.so*

%{PrefixDir}/%{_lib}/*
%{PrefixDir}/bin/*
%{PrefixDir}/sbin/*
%{PrefixDir}/include/*

%changelog
