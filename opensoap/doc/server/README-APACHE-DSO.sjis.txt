OpenSOAPサーバのApache DSOモジュールについて
============================================

OpenSOAPサーバでは、HTTPトランスポートの受信インターフェイスとして、
ApacheのCGIを使って呼び出されます。
メッセージのトランザクションが増大すると、この呼び出し部分がオーバーヘッ
ドとなり処理能力が低下するため、apacheのDSO(Dynamic Shared Object)に対
応させました。これにより、20%程度の性能の向上がありました。

また、従来扱えなかった、添付データを含むメッセージを扱うことができるよ
うになっています。
WS-Attachments 及び SOAP with Attachments(SwA)に準拠した DIME / MIME
方式の添付メッセージの転送が可能になりました。

このモジュール機能を使うためには、Apache2のDSOモジュールを利用できる環
境と、httpd.conf内での追加設定が必要となります。

--------------
コンパイル方法
--------------
まず、ソースコードからのコンパイルのためには httpd-devel相当のパッケー
ジがインストールされている必要があり、apxs (APache eXtenSion tool)コマ
ンドが正しく実行できる必要があります。
これらが揃っていれば、configureで自動認識されます。コンパイルの詳細は、
INSTALLドキュメントをご覧下さい。

正しくmakeが完了した場合には、作成されるモジュールファイル名は
mod_opensoap.so となり、 /usr/lib/httpd/modules/ (RedHat9の場合)
などにインストールされます。

--------
設定方法
--------
mod_opensoap機能を有効にするために httpd.conf 内に以下の記述を追加しま
す。

  #####The following lines need to be included in Apache2 httpd.conf
  #####for OpenSOAP Server DSO
  LoadFile /usr/local/opensoap/lib/libOpenSOAPServer.so
  LoadFile /usr/local/opensoap/lib/libOpenSOAPInterface.so
  LoadModule opensoap_module modules/mod_opensoap.so
  <Location /opensoap>
  SetHandler opensoap
  </Location>

LoadFileに記述する行は、インストールされるOpenSOAPサーバ用のライブラリ
ファイル名で、インストール手順により以下のように異なる可能性があります。
  /usr/local/opensoap/lib/libOpenSOAPServer.so
  /usr/lib/libOpenSOAPServer.so

また、RedHat7.3等では libstdc++ライブラリへのパス名が必要となる場合が
あります。
  # (libstdc++ is only for RedHat7.3)
  LoadFile /usr/lib/libstdc++-3-libc6.2-2-2.10.0.so

モジュール機能を有効するために、OpenSOAPサーバとともにhttpdサービスを
再起動して下さい。

例:
  # /etc/init.d/httpd restart
  # /etc/init.d/opensoap restart
 (あるいは、)
  # apachectl restart
  # opensoap-server-ctl restart

------------------
アクセス・確認方法
------------------
DSOモジュール経由でアクセスする場合は、エンドポイントが、
soapInterface.cgi ではなく、http://ホスト名/opensoap に替わります。

  $ soaping http://localhost/opensoap
  SOAPING http://localhost/opensoap : 0 byte string.
  soaping-seq=0 time=14.619 msec
 
  --- http://localhost/opensoap soaping statistics ---
  1 messages transmitted, 1 received, 0% loss, time 14.619ms
  Round-Trip min/avg/max/mdev = 14.619/14.619/14.619/0.000 msec
