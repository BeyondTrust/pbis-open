四則演算サンプル
================

このディレクトリには、四則演算のクライアントおよび
サービスのサンプルコードが含まれています。
注）このサンプルは、MS Soap Toolkit との接続検証を目的としていたため、
　　サービスの Namespace が http://tempuri.org/message/
　　となっています。

サンプルの利用法）
OpenSOAP API インストール後は、make clean; make -f Calc.mak で
コンパイルでき、以下のファイルが作成されます。
また、Calc.makの記述に従って、make -f Calc.mak service-install で
サービスプログラム、SSMLファイル等のインストールが出来ます。
Calc.mak を編集し MSSOAPSAMPLESERVER の値を MS Soap Toolkit が
インストールおよび動作しているマシンに設定し、
make -f Calc.mak ctest1 等実行することにより、MS Soap Toolkit との
接続検証を行えます。

* CalcClient
  四則演算クライアントプログラムです。

* CalcService
  標準入出力を用いた四則演算サービスプログラムです。inetd を用いることで、
  socket type のサービスにすることが可能です。

* CalcService.cgi
  CGI 型の四則演算サービスプログラムです。
