# ex: set tabstop=4 expandtab shiftwidth=4:

Name: 		@PKG_RPM_NAME@
Summary: 	Likewise CIFS Server
Version: 	@PKG_RPM_VERSION@
Release: 	@PKG_RPM_RELEASE@
License: 	HP
URL: 		http://www.likewise.com/
Group: 		Development/Libraries

Prereq: grep, sh-utils
Obsoletes:   likewise-open-libs, likewise-open-lsass, likewise-open-netlogon, likewise-open-lwio, likewise-open-eventlog, likewise-open-rpc, likewise-open-lwsm, likewise-open-lwreg, likewise-open-srvsvc

%if @PKG_RPM_COMPAT@
%package compat
Summary:        Likewise CIFS Server (compat libraries)
Group:          Development/Libraries
Requires:       @PKG_RPM_NAME@
%endif

%package devel
Summary:        Likewise CIFS Server (development)
Group:          Development/Libraries
Requires:       @PKG_RPM_NAME@

%description
The Likewise-CIFS server is a complete SMB/CIFS server stack
including standalone modes and membership in an AD domain,
user authentication, and DCE/RPC services.

%if @PKG_RPM_COMPAT@
%description compat
This package provides compatibility with 32-bit applications
%endif

%description devel
This package provides files for developing against the Likewise APIs

%post
case "$1" in
    1)
    ## New install
    if [ "@IS_EMBEDDED@" = "no" ]
    then
        %{_bindir}/domainjoin-cli configure --enable pam
        %{_bindir}/domainjoin-cli configure --enable nsswitch
    fi

    ## chkconfig behaves differently on various updates of RHEL and SUSE
    ## So, we massage the init script according to the release, for now.
    for d in lsassd lwsmd lwiod eventlogd dcerpcd netlogond lwregd srvsvcd; do
	daemon="/etc/init.d/$d"
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

    /etc/init.d/lwsmd start
    echo -n "Waiting for lwreg startup."
    while( test -z "`/opt/likewise/bin/lwsm status lwreg | grep standalone:`" )
    do
        echo -n "."
        sleep 1
    done
    echo "ok"
    for file in %{_prefix}/share/config/*.reg; do
        echo "Installing settings from $file..."
        %{_bindir}/lwregshell import $file
    done
    /etc/init.d/lwsmd reload
    sleep 2
    %{_bindir}/lwsm start srvsvc
    ;;

    2)
    ## Upgrade

    ## chkconfig behaves differently on various updates of RHEL and SUSE
    ## So, we massage the init script according to the release, for now.
    for d in lsassd lwsmd lwiod eventlogd dcerpcd netlogond lwregd srvsvcd; do
	daemon="/etc/init.d/$d"
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

    [ -z "`pidof lwsmd`" ] && /etc/init.d/lwsmd start

    echo -n "Waiting for lwreg startup."
    while( test -z "`/opt/likewise/bin/lwsm status lwreg | grep standalone:`" )
    do
        echo -n "."
        sleep 1
    done
    echo "ok"

    for file in %{_prefix}/share/config/*.reg; do
        echo "Upgrading settings from $file..."
        %{_bindir}/lwregshell import $file
    done
    /etc/init.d/lwsmd reload
    sleep 2
    %{_bindir}/lwsm stop lwreg
    %{_bindir}/lwsm start @PRIMARY_SERVICE@
    ;;

esac

%preun
if [ "$1" = 0 ]; then
    ## Be paranoid about cleaning up
    if [ "@IS_EMBEDDED@" = "no" ]
    then
        %{_bindir}/domainjoin-cli configure --disable pam
        %{_bindir}/domainjoin-cli configure --disable nsswitch
    fi

    %{_bindir}/lwsm stop lwreg
    /etc/init.d/lwsmd stop
fi

%changelog


