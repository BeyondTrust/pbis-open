=begin
= はじめに
 このディレクトリ以下にあるファイル郡は、win32 環境でのコンパイルに
使用する、Visual Studio のプロジェクトファイルおよびワークスペース
ファイルです。

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
 OpenSOAP\OpenSOAP.dsw を開き、コンパイルする。

 Server環境はOpenSOAPServer\READMESJ.TXTを参照。
=end
