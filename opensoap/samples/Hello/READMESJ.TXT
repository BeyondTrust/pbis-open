Hello サンプル
==============

このディレクトリには、Hello クライアントおよびサービスの
サンプルコードが含まれています。

OpenSOAP API インストール後は、make clean したあとで、
make -f Hello.mak でコンパイルして下さい。
以下のファイルが作成されます。

* HelloClient
  Hello クライアントプログラムです。
  HelloClient.c 内の OpenSOAPTransportSetURL() の二つめの引数を
  変更することにより、OpenSOAPサーバを経由する HelloService と、 
  HelloService.cgi のどちらかのをサービスを撰択できます。

* HelloService
  標準入出力を用いた Hello サービスプログラムです。

* HelloService.cgi
  CGI 型の Hello サービスプログラムです。cgi 実行可能なディレクトリにコピー
  することで利用可能となります。

サンプルの実行の仕方等は、Hello.mak を参照してください。
