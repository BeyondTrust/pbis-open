同期／非同期／ホップ数／転送パス指定 四則演算サンプル
=====================================================

このディレクトリには、四則演算サービスと、OpenSOAPサーバのメッセージン
グ機能（同期／非同期及び、サーバ転送時の最大ホップ数と転送ホスト名指定）
を利用できるクライアントのサンプルコードが含まれています。
クライアントにはコマンドライン版とWebブラウザ版があります。

OpenSOAP API インストール後は、make -f CalcAsync.mak で、
コンパイルでき、以下のファイルが作成されます。

* CalcAsyncClient
  コマンドライン版の四則演算クライアントプログラムです。
* CalcAsyncApp.cgi
  CGI 型の四則演算クライアントプログラムです。Webブラウザ上で
  実行することが可能です。
* CalcAsyncService
  標準入出力を用いた四則演算サービスプログラムです。
  inetd を用いることで、socket type のサービスにすることが可能です。
* CalcAsyncService.cgi
  CGI 型の四則演算サービスプログラムです。

ルート権限で、make -f CalcAsync.mak install を実行すると、サービスプロ
グラムと SSMLファイル、及び クライアントプログラムをインストールします。
新規にSSMLファイルをインストールした場合には、
# opensoap-server-ctl reload
で、設定ファイルを読み直して下さい。


コマンドライン版 CalcAsyncClient の使い方は以下のようになります。

Usage: CalcAsyncClient [options] method [op_a] [op_b]
  method         Add[Async], Subtract[Async], Multiply[Async], Divide[Async]
                 or GetResult.
  op_a           first operand (except for GetResult)
  op_b           second operand (except for GetResult)

 [options]
  -s endpoint    SOAP service endpoint URI
  -a soapaction  value of SOAPAction HTTP-Header
  -c char_enc    character encoding of request message (default: UTF-8)

  (messaging options)
  -H hopcount             set hopcount (>= 0)
  -f path1[,path2[,...]]  set forward path by URI (Commas are not allowed)
                          e.g.http://host/cgi-bin/soapInterface.cgi
  -F forward_path_file    set forward path from input file

op_a と op_b の2つの数字と、4種類の計算方法、
Add(足し算) Subtract(引き算) Multiply(掛け算) Divide(割り算)
を指定します。

$ ./CalcAsyncClient Add 1 2
  (同期モードで 1 + 2 を計算)

サーバの非同期モードで実行する場合は、計算方法に Async をつけます。
非同期では計算結果はすぐには返らず、サーバ側で処理が行われ、別途処理結
果を問い合わせる必要があります。

$ ./CalcAsyncClient MultiplyAsync 1 2 > tmp_msg
  (非同期モードで 1 * 2 を計算)
$ ./CalcAsyncClient GetResult < tmp_msg
  (返されるメッセージIDを元に結果を問い合わせる)

コマンドラインオプションでサービスのエンドポイントを指定できます。
hostname上のOpenSOAPサーバでサービスを実行する場合には
-s http://hostname/cgi-bin/soapInterface.cgi
となります。

指定したOpenSOAPサーバ上にサービスがない場合、そのサーバは別のサーバに
メッセージを転送して処理を依頼します。
どこまで転送をするかの最大ホップ数を -H オプションで指定できます。

$ ./CalcAsyncClient -s http://host/cgi-bin/soapInterface.cgi -H 4 AddAsync 1 2
  (サーバ:host にサービスがない場合、4回まで転送される)

転送するサーバをクライアント側が指定することもできます。
ファイルから読み込む(-F)か、コマンドライン(-f)から指定できます。

$ cat forward_path.txt
http://host1/cgi-bin/soapInterface.cgi
http://host2/cgi-bin/soapInterface.cgi

$ ./CalcAsyncClient -s http://host/cgi-bin/soapInterface.cgi \
                    -F forward_path.txt  AddAsync 1 2
  (ファイルから指定：サーバhostにサービスがない場合、host1, host2 と順
  に転送される)

$ ./CalcAsyncClient -s http://host/cgi-bin/soapInterface.cgi \
  -F http://host1/cgi-bin/soapInterface.cgi,http://host2/cgi-bin/soapInterface.cgi \
  AddAsync 1 2
  (コマンドラインオプションで指定：同様にhost1, host2 と転送)

CalcAsync.mak に簡単なテスト用のルールが用意してあります。

$ make -f CalcAsync.mak stest1
  (サンプルリクエストメッセージを使ったサービス機能のテスト)
$ make -f CalcAsync.mak ctest1
  (クライアント機能のテスト)
$ make -f CalcAsync.mak cstest1
  (サービス・クライアント機能の連携のテスト)
$ make -f CalcAsync.mak cstest1a
$ make -f CalcAsync.mak cstestr
  (サービス・クライアント機能の非同期でのテスト。
   cstestrで処理結果を問い合わせできます。)


同様の機能を CGI版のクライアントを使って Webブラウザ上で実行できます。
デフォルトでインストールされると、次のURLから処理を行います。
http://localhost/cgi-bin/CalcAsyncApp.cgi

クライアントで送受信されるSOAPメッセージの内容を確認したい場合は、
"Display Request & Response messages" をチェックして下さい。
