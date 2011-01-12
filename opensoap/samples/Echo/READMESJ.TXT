Echo サービス・クライアント
===========================

このプログラムは単純に値の受け渡しを行うことで、さまざまな動作確認がで
きることを目的としています。
さまざまな型を細かく扱えるようにして、また、Hello系サンプルとは逆にエ
ラーチェックをなるべく行ない、各種API拡張機能のデバッグに使うような処
理を盛り込んであります。
最終的には、相互接続性試験や、リリース前のリグレッションテスト向けに自
動化して使えるようにしたいと考えています。

* EchoClient
  Echo クライアントプログラムです。

  使用方法: EchoClient [options] test# data [data2 [data3 ..]]
  オプション:
  -h --help              ヘルプメッセージ表示
  -v --verbose           冗長モード。SOAPメッセージを表示。
  -d --debug             デバッグモード。デバッグメッセージを表示。
  -s --endpoint=ENDPOINT EchoServiceのエンドポイント
                         (default: http://localhost/cgi-bin/EchoService.cgi)
  -u --http-user=USER    HTTP認証のユーザ名
                         (default: env ECHO_HTTP_USER)
  -p --http-passwd=PASS  HTTP認証のパスワード
                         (default: env ECHO_HTTP_PASS)
  -t --http-type=AUTH    HTTP認証タイプ (デフォルト:0=何でも,
                         1=ベーシック, 2=ダイジェスト)
  -S --sslver=VER        使用するSSLバージョンを指定
                         (default:0=ALL, 1=SSLv2,2=SSLv3,4=TLSv1)
  -V --verify=LEVEL      SSLサーバ認証レベルを指定
                         (default:0=気にしない, 1=厳密)
  --ca_dir=DIR           CA証明書の含まれるディレクトリを指定
  --ca_file=FILE         CA証明書のファイル名を指定
  --certchain_file=FILE  クライアント認証チェインのファイル名
  --privkey_file=FILE    クライアント認証の秘密鍵のファイル名

 test# = メソッド名
 test:0 = Echo
  送られてきたメッセージをそのまま返す。※未実装※

 test:1 = echoString
  文字列を指定して送る。

  例$ EchoClient 1 foo

 test:2 = echoStringArray
  文字列の配列を指定して送る。※未実装※

  例$ EchoClient 2 foo bar

 test:3 = echoInteger
  整数を指定して送る。

  例$ EchoClient 3 1234567890

 test:4 = echoIntegerArray
  整数の配列を指定して送る。※未実装※

  例$ EchoClient 3 1 2 3 4 5 6 -1 -2 -3

 test:5 = echoFloat
  浮動小数点数を指定して送る。

  例$ EchoClient 5 3.141592

 test:6 = echoFloatArray
  浮動小数点数の配列を指定して送る。※未実装※

  例$ EchoClient 6 1.1 1.2 1.3 1.4 1.5

 test:7 = echoStruct
  構造体のデータを指定して送る。※未実装※

  例$ EchoClient 7 filename?

 test:8 = echoStructArray
  構造体のデータの配列を指定して送る。※未実装※

  例$ EchoClient 8 filename1 filename2 filename3？

 test:9 = echoVoid
  voidを送る。※未実装※

  例$ EchoClient 9

 test:10 = echoBase64
  指定したファイルの内容をBase64エンコーディングして送る。

  例$ EchoClient 10 ファイル

 test:11 = echoDate
  指定した日付けを送る。指定しない場合、現在時刻。

  例$ EchoClient 11
  例$ EchoClient 11 "Fri Nov  7 11:50:23 JST 2003"

 test:12 = echoDouble
  倍精度浮動小数点数を指定して送る。

  例$ EchoClient 5 3.1415926535897932384626433832795029

* EchoService
  標準入出力型の Echo サービスプログラム

* EchoService.cgi
  CGI型の Echo サービスプログラム

値を一度型を合わせた変数に入れてから処理するので、型と値が合わないとエ
ラーになる可能性があります。
