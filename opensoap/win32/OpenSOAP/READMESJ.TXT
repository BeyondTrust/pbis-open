=begin
= はじめに
 このディレクトリにあるファイル郡は、win32 環境でAPIのコンパイルに
使用する、Visual Studio のプロジェクトファイルおよびワークスペース
ファイルです。

= ファイルリスト
 このディレクトリには以下のファイルが含まれている。
  * READMESJ.TXT		このファイル。
  * OpenSOAP.dsw		API 構築用ワークスペースファイル
  * *.dsp			API 構築用プロジェクトファイル郡
  * OpenSOAPAPIInstall.bat	API インストール用 BAT ファイル
  * Makefile*			OpenSOAP パッケージ作成用ファイル

= コンパイル前の準備
このパッケージのコンパイルには、以下のものが必要です。

 (1) libxml2
 (2) openssl

== libxml2 の準備
 libxml2 は、まず、以下のディレクトリを作成する。
  * C:\LIBXML2
  * C:\LIBXML2\BIN
  * C:\LIBXML2\LIB
 パッケージ(libxml2-verion.tar.gz として)の入手後、展開し、
libxml2-version\win32\libxml2\libxml2_so.dsp を用いて、コンパイルし、
libxml2-version\include を ディレクトリごと C:\LIBXML2 に、
libxml2-version\win32\libxml2\libxml2_so\libxml2.dll を、C:\LIBXML2\BIN に、
libxml2-version\win32\libxml2\libxml2_so\libxml2.lib を、C:\LIBXML2\LIB に
それぞれコピーし、C:\LIBXML2\BIN へパスを設定する。

== openssl の準備
 openssl はドキュメントに従い、MSC版をC:\OPENSSL へインストールする。

= コンパイル方法
 OpenSOAP.dsw を開き、コンパイルする。ビルド(B)→バッチビルド(U)...
 でバッチビルドのダイアログを開き、プロジェクトの構成(P)にチェックし、
 リビルド(R)ボタンを押す事でAPIの構築が可能である。

= インストール
 OpenSOAPAPIInstall.bat を実行する。これにより、C:\OpenSOAP 以下にファイルが
 インストールされる。インストール後、C:\OpenSOAP\bin を PATH に設定すること
 により、OpenSOAP API を DLL として利用したアプリケーションの実行が可能となる。

= 利用方法
 Visual Studio にて、プロジェクト(P)→設定(S)... で、プロジェクトの設定
 ダイアログを開き、C/C++タブを選択し、カテゴリ(Y)をプリプロセッサにして、
 インクルードファイルのパス(N)に、C:\OpenSOAP\include を追加する。
 また、カテゴリをコード生成にし、使用するランタイムライブラリ(L)を
 マルチスレッド(DLL)に変更する。
 次にリンクタブを選択し、カテゴリをインプットにする。
 オブジェクト／ライブラリモジュール(M)にライブラリを追加する。
 追加するライブラリは作成するアプリケーションにより異なる。
 Client を作成する場合は、OpenSOAPClient.lib を追加する。
 Service を作成する場合は、これに加えて、OpenSOAPService.lib を追加する。
 また、セキュリティモジュールを使用する場合は、OpenSOAPSecurity.lib も
 追加する。
 次に、追加ライブラリのパス(D)にライブラリのパスを追加する。
 DLL を使用する場合は、C:\OpenSOAP\lib\DLL を追加する。スタティックライブラリ
 を使用する場合は、C:\OpenSOAP\lib を追加する。両方追加した場合は、先に
 追加した方が優先されるので、注意する事。
 あと、Service を作成する場合、Console アプリケーションとして作成すること。

=end
