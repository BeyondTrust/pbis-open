# -*- mode: rpm-spec; -*-

%define version_major  2.0
%define version_relext 1

%define version_package 2.0

## for RedHat Linux
%define _with_cgi_bin /var/www/cgi-bin
## for Vine Linux
#define _with_cgi_bin /home/httpd/cgi-bin

%define _sysconfdir /etc/opensoap
%define _with_servicesdir /usr/lib/opensoap

%define _localstatedir /var/opensoap
# may need to be modified like /var/{log,spool,run}/opensoap

Summary: SOAP-based middleware for XML Web Services applications.
Summary(ja): XML Webサービスアプリケーション向けSOAPベースミドルウェア。
Name: opensoap
Version: %{version_major}
Release: %{version_relext}
## for Vine Linux
#Release: %{version_relext}vl1
Epoch: 1
Group: System Environment/Libraries
Group(ja): システム環境/ライブラリ
BuildRequires: libxml2-devel, openssl-devel, httpd-devel
Requires: libxml2, openssl
Source: www.opensoap.jp:/download/%{name}-%{version_package}.tar.gz
Copyright: BSD
URL: http://www.opensoap.jp/
BuildRoot: %{_tmppath}/%{name}-buildroot
#Patch: %{name}-%{version_package}.patch

%description
OpenSOAP provides an environment for XML Web Services.
API libraries to use SOAP (Simple Object Access Protocol) and the
server for service applications are provided.  Also Transaction and
Security functions are implemented.
This main package includes shared libraries for executing OpenSOAP
application software, and such basic tools as Soaping client/service
and Transaction service.

%description -l ja
OpenSOAPはXML Webサービスのための環境を提供します。SOAP (Simple Object
Access Protocol)を利用するためのAPIライブラリと、サービスアプリケーショ
ン運用のためのサーバを提供し、トランザクションやセキュリティの機能を持っ
ています。
この本体パッケージには、OpenSOAPアプリケーションソフト実行時に必要な
共有ライブラリ、及び、基本ツールであるSoapingクライアント/サービス、
Transactionサービスが含まれます。

%package devel
Summary: Libraries, include files to develop OpenSOAP applications.
Summary(ja): OpenSOAPアプリケーション開発のためのライブラリとヘッダファイル。
Group: Development/Libraries
Group(ja): 開発/ライブラリ
Requires: %{name} = %{version}

%description devel
Libraries, include-header files and documentation which can be used to
develop software based on SOAP for Web Services, and to compile the
source code for OpenSOAP application programs.

%description devel -l ja
このライブラリとヘッダファイル、及びドキュメント類を利用することで、
WebサービスのためのSOAP通信を利用するソフトウェアを開発したり、入手し
たOpenSOAPアプリケーションのソースコードをコンパイル及びリンクすること
が可能となります。

%package server
Summary: Server for the original OpenSOAP functions.
Summary(ja): OpenSOAP独自の機能を実現するためのサーバ。
Group: System Environment/Daemons
Group(ja): システム環境/デーモン
Requires: %{name} = %{version}

%description server
The OpenSOAP Server supports operations for Web Service systems and
provides practical functions, such as asynchronous client connections
for non-real time processing, message forwarding for searching valid
services and passing through firewalls, and signing messages for
security validation.

%description server -l ja
OpenSOAPサーバは、大規模なWebサービスシステムの運用をサポートします。
サービスの非リアルタイム処理に対応する非同期クライアント接続機能、サービ
スの探索やファイアウォール越えを実現するサーバ間メッセージ転送機能、メッ
セージへの署名などを行うセキュリティ機能が含まれています。

%package samples
Summary: Sample application programs of OpenSOAP.
Summary(ja): OpenSOAPのサンプルアプリケーションプログラム。
Group: Applications/Networking
Group(ja): アプリケーション/ネットワーク
Requires: %{name} = %{version}

%description samples
This is a collection of simple sample application programs using
OpenSOAP.  Please try them if you just want to use OpenSOAP.  If you
are interested in developing application programs, please also refer
to the source codes.

%description samples -l ja
OpenSOAPアプリケーションの簡単なサンプルの寄せ集めです。
OpenSOAPをとりあえず使ってみたい場合に、是非お試し下さい。アプリケーショ
ンプログラム開発に興味のある方は、ソースコードもご覧下さい。

%prep
%setup -q -n %{name}-%{version_package}
#%patch -p1

rm -rf $RPM_BUILD_ROOT

%build
%configure --sysconfdir=%{_sysconfdir} --with-servicesdir=%{_with_servicesdir} --with-cgi-bin=%{_with_cgi_bin} --localstatedir=%{_localstatedir} --with-ssl-include=/usr/kerberos/include

## doc files to be copied
cp -pr doc docs
find docs -name 'Makefile*' -exec rm {} \;
rm -rf docs/api/*/Doxyfile
rm -rf docs/api/*/text/
cp -pr samples docs/

make

%install
LIBRARY_PATH="$RPM_BUILD_ROOT"%{_libdir} make DESTDIR="$RPM_BUILD_ROOT" install
# samples
make DESTDIR="$RPM_BUILD_ROOT" install-samples

# install SYSV init stuff
mkdir -p $RPM_BUILD_ROOT/etc/rc.d/init.d
install -m755 etc/init.d/opensoap.redhat \
	$RPM_BUILD_ROOT/etc/rc.d/init.d/opensoap

%clean
rm -rf $RPM_BUILD_ROOT

%post server
# Register the opensoap service
/sbin/chkconfig --add opensoap

%preun server
if [ $1 = 0 ]; then
#	%{_sbindir}/opensoap-server-ctl stop || exit 0
	/sbin/service httpd stop
	/sbin/service opensoap stop
	/sbin/chkconfig --del opensoap
fi

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%defattr(-,root,root)

%{_bindir}/soaping
%{_libdir}/libOpenSOAPClient.so.*
%{_libdir}/libOpenSOAPService.so.*
%{_libdir}/libOpenSOAPSecurity.so.*
%{_with_cgi_bin}/TransactionService.cgi
%{_with_cgi_bin}/SoapingService.cgi
%{_with_servicesdir}/Soaping
%{_with_servicesdir}/Transaction
%{_localstatedir}/services/Transaction
%{_mandir}/man1/*
%{_mandir}/ja/man1/*
%doc INSTALL INSTALL.ujis README README.ujis LICENSE
%doc HISTORY HISTORY.ujis TODO TODO.ujis
%doc docs/tools

%files devel
%defattr(-,root,root)

%{_includedir}/OpenSOAP
%{_libdir}/libOpenSOAPClient.so
%{_libdir}/libOpenSOAPService.so
%{_libdir}/libOpenSOAPSecurity.so
%{_libdir}/libOpenSOAPClient.*a
%{_libdir}/libOpenSOAPService.*a
%{_libdir}/libOpenSOAPSecurity.*a
%doc docs/api/en
%doc docs/api/ja
#%doc docs/security
#%doc docs/java

%files server
%defattr(-,root,root)

%{_sbindir}/*
%{_libdir}/libOpenSOAPServer.*
%{_libdir}/libSOAPMessage.*
%{_libdir}/libconnection.*
%{_libdir}/libOpenSOAPInterface.*
%{_libdir}/libFileLib.*
%{_libdir}/libSharedLib.*
%{_libdir}/libTraceLib.*

%config %{_sysconfdir}/server.conf
%config %{_sysconfdir}/privKey.pem
%config %{_sysconfdir}/pubKey.pem
%{_sysconfdir}/*pem.default
%config %{_sysconfdir}/ssml/Soaping.ssml
%config %{_sysconfdir}/ssml/Transaction.ssml
%{_with_cgi_bin}/soapInterface.cgi
%{_mandir}/man8/*
%{_mandir}/ja/man8/*
%doc docs/server/*
%config /etc/rc.d/init.d/opensoap
/usr/lib/httpd/modules/mod_opensoap.so
%doc etc/server/mod_opensoap/httpd-mod_opensoap.conf

%files samples
%defattr(-,root,root)

%doc docs/samples/*
%{_with_cgi_bin}/*Calc*
%{_with_cgi_bin}/Hello*
%{_with_cgi_bin}/GetCertService.cgi
%{_with_cgi_bin}/*Shopping*
%{_with_cgi_bin}/TransactionApp.cgi
%{_with_cgi_bin}/TransactionHtml
%{_with_cgi_bin}/TransactionABankService.cgi
%{_with_cgi_bin}/Echo*
%{_with_servicesdir}/*Calc*
%{_with_servicesdir}/*Hello*
%{_with_servicesdir}/GetCert
%{_with_servicesdir}/*Shopping*
%{_with_servicesdir}/TransactionABank
%{_with_servicesdir}/Echo
%{_bindir}/*Calc*
%{_bindir}/*Hello*
%{_bindir}/GetCertClient
%{_bindir}/registCA
# %{_bindir}/genrsakey
# %{_bindir}/secEnv
%{_bindir}/*Shopping*
%{_bindir}/TransactionClient
%{_bindir}/EchoClient
%config %{_sysconfdir}/ssml/*Calc*
%config %{_sysconfdir}/ssml/GetCert.ssml
%config %{_sysconfdir}/ssml/*Hello*
%config %{_sysconfdir}/ssml/*Shopping*
%config %{_sysconfdir}/ssml/TransactionABank.ssml
%config %{_sysconfdir}/ssml/Echo.ssml
%{_localstatedir}/services/CalcAsync
%{_localstatedir}/services/GetCert
%{_localstatedir}/services/ShoppingSec

%changelog
* Mon Mar 11 2004 Nobuhito OKADA <okada@opensoap.jp> 1.1-0.20040311
- merged many new features.

* Tue Jan 27 2004 Nobuhito OKADA <okada@opensoap.jp>
- stop httpd daemon at pre-uninstallation of server 

* Fri Jan 10 2004 Nobuhito OKADA <okada@opensoap.jp>
- added files for Apache DSO modules of OpenSOAP Server
- added new library files

* Mon Dec 01 2003 Nobuhito OKADA <okada@opensoap.jp> 1.0-0.20031201
- added /usr/kerberos/include to avoid ssl compilation errors on RedHat9

* Wed Sep 03 2003 Nobuhito OKADA <okada@opensoap.jp> 1.0-0.20030903
- specify LIBRARY_PATH for make install

* Fri May 30 2003 Nobuhito OKADA <okada@opensoap.jp> 1.0-0.20030530
- added init script
- added Epoch because versioning was confusing
- added man/ja/
- build opensoap-samples properly

* Sat Mar  1 2003 Nobuhito OKADA <okada@opensoap.jp> 1.0_20030228-2
- added %defattr
- removed Vendor
- corrected _with_cgi_bin (bug of define ?)

* Thu Feb 28 2003 Nobuhito OKADA <okada@opensoap.jp> 1.0_20030228-1
- Released as the first version.
