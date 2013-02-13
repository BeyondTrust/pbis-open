CORBAExeCalc Sample( CORBA EXE Server )
(簡易演算のCORBAサービス(EXE系)を利用するCORBAブリッジングサンプル )

このディレクトリには、CORBAExeCalc クライアントおよびサービスの
サンプルコードが含まれています.

1. OpenSOAP API インストール後は、make -f CORBAExeCalc.mak で
コンパイルでき、以下のファイルが作成されます.

* CORBAExeCalcClient
  CORBAExeCalc クライアントプログラムです.
 
* CORBAExeCalcService
  標準入出力を用いた  CORBAExeCalc サービスプログラムです.inetd を用いることで、
  socket type のサービスにすることが可能です.

* CORBAExeCalcService.cgi
  CGI 型の CORBAExeCalc サービスプログラムです. cgi 実行可能なディレクトリにコピー
  することで利用可能となります.

2. サンプルの実行の仕方:

## Test Service Programs

stest:
	cat ../RequestMessage/request_add.xml | ./CORBAExeCalcService.cgi

## Test Client-Service (or Client-Server-Service) Hookups

ctest:
	./CORBAExeCalcClient -s http://localhost/cgi-bin/CORBAExeCalcService.cgi 12.9 + 10.0 
