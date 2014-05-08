CORBADllCalc Sample
(簡易演算のCORBAサービス(DLL系)を利用するCORBAブリッジングサンプル )

このディレクトリには、CORBADllCalc クライアントおよびサービスの
サンプルコードが含まれています.

1.OpenSOAP API インストール後は、make -f CORBADllCalc.mak で
コンパイルでき、以下のファイルが作成されます.

* CORBADllCalcClient
  CORBADllCalc クライアントプログラムです.
 
* CORBADllCalcService
  標準入出力を用いたCORBADllCalc サービスプログラムです。inetd を用いることで、
  socket type のサービスにすることが可能です.

* CORBADllCalcService.cgi
  CGI 型の CORBADllCalc サービスプログラムです. cgi 実行可能なディレクトリにコピー
  することで利用可能となります.

2. サンプルの実行の仕方:

## Test Service Programs

stest:
	cat ../RequestMessage/request_add.xml | ./CORBADllCalcService.cgi

## Test Client-Service (or Client-Server-Service) Hookups

ctest:
	./CORBADllCalcClient -s http://localhost/cgi-bin/CORBADllCalcService.cgi 12.9 + 10.0 
	