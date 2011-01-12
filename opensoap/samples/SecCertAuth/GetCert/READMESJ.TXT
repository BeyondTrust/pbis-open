電子証明書取得サンプル
================

このディレクトリには、電子証明書取得のクライアントおよび
サービスのサンプルコードが格納されています。

OpenSOAP API インストール後は、make -f GetCert.mak で、
コンパイルでき、以下のファイルが作成されます。

* GetCertClient
  電子証明書取得クライアントプログラムです。
* GetCertService
  標準入出力を用いた電子証明書取得サービスプログラムです。inetd を用いることで、
  socket type のサービスにすることが可能です。
* GetCertService.cgi
  CGI 型の電子証明書取得サービスプログラムです。
* CA_Refer
  実際に認証局データベースにアクセスして証明書取得するプログラムです。
  GetCertService, GetCostService.cgi から呼び出されるプログラムです。

※使用方法、各種設定について

* GetCertClient(電子証明書取得クライアントプログラム)
  プログラムの引数に、鍵の所有者名称を与えて起動することで、電子証明書と
  公開鍵を取得できます。それぞれ、"result.cert" と "pubKey_res.pem"
  という名前で生成されます。
  これらの名称を変更したい場合は、 GetCertClient.c 中のそれぞれ 
  RESULT_CERT_FILE_NAME, RESULT_PUBKEY_FILE_NAME という定義を変更すれば
  OKです。
  また、得られた電子証明書の署名検証のために、認証局の公開鍵が必要となり
  ますが、このファイルは GetCertClient.c 中に CA_PUBKEY_FILE として定義
  してありますので、必要に応じて修正利用下さい。(現状の定義は、OpenSOAP
  Server がインストールされた時に設定されるものを利用する様に設定されて
  います)

* GetCertService(電子証明書取得サービスプログラム)
  実際に検索を実行するプログラム(CA_Refer)がインストールされているディレ
  クトリとして、GetCertService.c 中に REFER_PROGRAM_DIR として定義してい
  ます。CA_Refer を他のディレクトリにインストールした場合は、この部分を
  変更して下さい。

* CA_Refer(認証局データベースでの探索プログラム)
  設定変更は、 DBRefer.c 内の以下の定義を操作します。

  CA_DATABASE_FILE : 認証局DBファイルのインストール先のフルパス
  CA_PRIVKEY_FILE  : 認証局の秘密鍵ファイル名

なお、このディレクトリ下にサンプルの認証局データベース(CA.db)を置いて
います。
このデータベースには、以下の所有者名称に対応する公開鍵が格納されています。
(３件)

"OpenSOAP-SAMPLE-CA" (認証局自身を示す)
"David Beckham"
"Alessandro Del Piero"

これは、Regist サンプルで作成されるものと同一となります。

                                                                        以上