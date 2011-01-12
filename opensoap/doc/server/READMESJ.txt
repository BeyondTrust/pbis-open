「OpenSOAP Server の動作手順」

(0) OpenSOAP Server の実行には http サーバーが必要です。
現在は apache httpd で動作を確認しています。

(1) INSTALL.ujis に従い、ビルドとインストールを行う。
apache httpd と OpenSOAP Server をつなぐ
soapInterface.cgi のインストールされるディレクトリに注意してください。
デフォルトでは /home/httpd/cgi-bin なので、必要に応じて
--with-cgi-bin で CGI が実行可能なディレクトリを指定してください。

(2) 標準ではサービスは標準入出力による接続を利用しますが、SSMLの設定により
ソケットによる接続も可能です。その場合は、RegistService.sjis.txt に従い、
サービスを inetd (xinetd) に登録する必要があります。
サービスプログラムを root 権限で実行するとエラーになることが、
以前報告されています。

さらに、SSMLの設定によりHTTPを利用したエンドポイントの指定によるサービス
接続もサポートしました。SSML_Readme.sjis.txtを参照してください。

(3) /usr/local/opensoap/etc/ 以下にある
サーバーの設定ファイル server.conf を編集する。

* 各サーバプロセスのログファイルの出力先の設定
  <log>
    <path>/usr/local/opensoap/var/log/</path>
  </log>

* サーバプロセスが内部管理するSOAPメッセージの保持場所の設定
  <spool>
    <soap_message>
      <path>/usr/local/opensoap/var/spool/</path>
    </soap_message>

* サーバプロセスが内部管理する非同期メッセージ管理テーブルの保持場所の設定
    <async_table>
      <path>/usr/local/opensoap/var/spool/</path>
    </async_table>
  </spool>

* 各サーバプロセスのプロセスID管理場所の設定
  <run>
    <pid>
      <path>/usr/local/opensoap/var/run/</path>
    </pid>

* 各サーバプロセスのソケット管理場所の設定
    <socket>
      <path>/usr/local/opensoap/var/run/</path>
    </socket>
  </run>

* サーバ内部署名認証用セキュリティキーの管理場所の設定
  <security>
    <keys>
      <path>/usr/local/opensoap/etc/</path>
    </keys>
  </security>

* サービス管理用SSMLファイルの登録先の設定
  <ssml>
    <path>/usr/local/opensoap/etc/ssml/</path>
  </ssml>

* 非同期転送時の返送メッセージが返信されるEndPointの設定
  ここで指定したEndPointに対して転送先から返信メッセージが送られるため、
  転送先から参照できる自サーバの情報である必要がある。
  また、メッセージ転送のループをチェックするために、自身の別名やIPがあれば
  それらを２つ目以降の<url>として記述しておく。

  以下の例の場合は、"http://myhost.opensoap.jp/cgi-bin/soapInterface.cgi"
  に対して返信される。

  <backward>
    <url>http://myhost.opensoap.jp/cgi-bin/soapInterface.cgi</url>
    <url>http://192.168.0.123/cgi-bin/soapInterface.cgi</url>
    <url>http://soap-server.opensoap.jp/cgi-bin/soapInterface.cgi</url>
  </backward>

* 自サーバ管理下に対象サービスが無い場合に、メッセージが転送される
  サーバのEndPointを<url>に設定する。

  以下の例の場合は、"http://yourhost.opensoap.jp/cgi-bin/soapInterface.cgi"
  に対して転送される。

  <forwarder>
    <url>http://yourhost.opensoap.jp/cgi-bin/soapInterface.cgi</url>
  </forwarder>

* 非同期メッセージのIDなど、サーバが制御するメッセージに対して
  サーバの署名を付加するか否かの設定
  サーバの署名を付加したい場合は true を設定する。

  <add_signature>false</add_signature>

* サーバが受付けるSOAPメッセージの最大サイズを指定する。
  単位はbyte. 500k, 1Mなどの指定も可能。
  -1などのマイナスの値を指定した場合は制限無し。
  0を指定した場合はメッセージを受付けない指定となる。

  <limit>
    <soap_message_size>1M</soap_message_size>
  </limit>

* 非同期処理における、未処理の内部キュー、スプールデータの破棄時間を
  指定する。
  0以下の値は無効。
  （未指定の場合はシステム内部デフォルト値3600が使用される）

  <limit>
    <ttl>
      <asynchronizedTTL>3600</asynchronizedTTL>
    </ttl>
  </limit>

  ※この値は、各サービス毎のSSMLファイルにも指定することが可能であり、
    SSMLファイルに指定がある場合は、そちらの値が優先される。

  ※また、個別のSOAPメッセージ中の拡張ヘッダにも同様の値が指定できるが、
    この場合は、対応するSSMLでの指定があれば、その値よりも小さい場合のみ
    有効となる。SSMLでの指定が無ければ、server.conf中の値よりも小さい場合
    に有効となる。

        設定例）                  
        server.conf | 10  10  10  10  10  10 
        SSML        | -   20   5  20  20  -
        SOAP-Header | -   -   -   30  15  15
        --------------------------------------
        有効値      | 10  20   5  20  15  10


* 同期処理における、それぞれのSOAPメッセージ処理タイムアウト時間を指定する。
  0以下の値は無効。
  （未指定の場合はシステム内部デフォルト値600が使用される）

  <limit>
    <ttl>
      <synchronizedTTL>600</synchronizedTTL>
    </ttl>
  </limit>

  ※SSMLファイル中の指定および、SOAPメッセージ中の拡張ヘッダでの指定の
    扱いは、上記 <asynchronizedTTL>と同じ。

* SOAPメッセージの転送回数の制限を指定する。
  0を指定した場合は、転送を行わない。
  マイナスの値は無効。
  （未指定の場合はシステム内部デフォルト値4が使用される）

  <limit>
    <ttl>
      <hoptimes>4</hoptimes>
    </ttl>
  </limit>

  ※SSMLファイル中の指定および、SOAPメッセージ中の拡張ヘッダでの指定の
    扱いは、上記 <asynchronizedTTL>と同じ。

* ログ出力に関する設定の一覧を以下に示す。

  <Logging><System><LogType>
	ログの種類。syslog=syslogへの出力。file=ファイルへの出力。
	デフォルト値はsyslog

  <Logging><System><LogFormat>
	出力内容。generic=概略出力。detail=詳細出力。
	デフォルト値はgeneric

  <Logging><System><Option>
	LogType=syslogの場合のオプション指定。syslogへの引数を指定する。
	必要なオプション値を足した値を設定。

	1=PID:		プロセスIDを表示する
	2=CONSOLE:	コンソールへ出力する
	4=ODELAY:	ログの初期化時にsyslogと接続する
	8=NDELAY:	ログの初回書き込み時にsyslogと接続する
	16=NOWAIT:	syslogへの書き込み時に子プロセス終了を待たずに終了
	32=PERROR:	stderrにも同時に出力する

	デフォルト値は5(=1+4)

  <Logging><System><Facility>
	LogType=syslogの場合のオプション指定。syslogファシリティーの設定。

	0=KERN
	8=USER
	24=DAEMON
	128=LOCAL0
	136=LOCAL1
	144=LOCAL2

	デフォルト値は8
	
  <Logging><System><DefaultOutputLevel>
	LogType=syslog用オプション。接続時のみ。
	現状実質未使用。現状開発用のため、変更不可。

  <Logging><System><LogLevel>
	共通オプション。ログ出力のレベル設定。指定以上のメッセージのみ出力。

	0=EMERG:	深刻なエラー
	2=ALERT:	アラートレベルのエラー
	3=ERR:		エラー
	4=WARN:		警告レベルのエラー
	5=NOTICE:	注意レベルのエラー
	6=INFO:		情報
	7-16=DEBUG:	追加捕捉レベル（デバッグ）

	デフォルト値は6

  <Logging><Application><LogType>
	<Logging><System><LogType>と同じ。
	デフォルト値はfile

  <Logging><Application><LogFormat>
	<Logging><System><LogFormat>と同じ。
	デフォルト値はdetail

  <Logging><Application><FileName>
	LogType=file用オプション。
	ログファイル名（完全パス指定）を指定する。

  <Logging><Application><LogLevel>
	<Logging><System><LogLevel>と同じ。

  ※以下の<プロセス名>を指定することにより、プロセス別にログファイルを分ける
    ことが可能となる。設定項目は<Logging><Application>と同じ
  <Logging><プロセス名>
	

(4) /usr/local/opensoap/etc/ssml/ 以下に
サービス設定ファイル SSML ファイルを置く。(server.confでの初期設定の場合)
SSML の記述法に付いては同ディレクトリ以下にある
SSML_Readme.txt を参照すること。

(5) apache httpd が起動していることを確認する。
起動していない場合は起動を行う。

(6) サーバープロセスの開始。
${sbindir} (デフォルトでは /usr/local/opensoap/sbin) にパスを通す。
(6-00) ${sbindir}/opensoap-server-ctl start を実行する。
※1: 停止する際は ${sbindir}/opensoap-server-ctl stop を実行する。
※2: SSML を追加した際は ${sbindir}/opensoap-server-ctl reload を実行する。
==============
 プロセス一覧
==============
※3: (6-00) では以下の (6-01)～(6-9) が実行される。
(6-01) OpenSOAPMgr
       - 以下の各サーバプロセスを管理（実行停止等）するマネージャプロセス
(6-02) srvConfAttrMgr 
         - サーバ設定情報管理機能プロセス
(6-03) ssmlAttrMgr 
         - SSML情報管理機能プロセス
(6-04) idManager
         - メッセージ内部管理識別子発行機能プロセス
(6-05) msgDrvCreator
         - メッセージ処理振り分け機能プロセス
(6-06) queueManager
         - メッセージキュー管理機能プロセス
(6-07) queueManager 1 
         - 転送用メッセージキュー管理機能プロセス
(6-08) spoolManager
         - 非同期レスポンスメッセージスプール機能プロセス
(6-09) ttlManager
         - メッセージ保持期間管理機能プロセス

※4: 不正にサーバプロセスが残ってしまった場合は、上記プロセス名を元に
    プロセスを終了させてください。


(7) クライアントプログラムを実行する。

＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝
「OpenSOAP サーバーのデバッグ・バグの報告」

(1) /usr/local/opensoap/var/log/ 以下に
各プログラムのログが書き出されるので、
どこでエラーになっているかチェックしてください。(server.conf初期設定時)

(2) configure のオプションに CXXFLAGS=-DDEBUG を付けると
詳細なログが出ます。

(3) 不正終了してしまったプログラムは
opensoap-server-ctl stop の際に名前が表示されません。
エラー報告の際に教えていただけると助かります。
