SSMLサンプル解説（Sample.ssml)

<?xml version='1.0' encoding='UTF-8' ?>
<SSML xmlns="x-schema:ssmlSchema.xml">

SSML(Soap Service Markup Language)の最上位要素は<SSML>であり，サービス名を
定義した<service>要素をひとつ含みます．
SSMLファイル名は便宜的にこのサービス名と同一にします．

<service name='Sample' nsuri='http://services.opensoap.jp/samples/Sample/'>

<service>ノードにはひとつ以上の<connection>要素と，任意の数の<operation>要素，
ひとつの<fault>要素，ひとつ以下の<MaxProccessNumber>要素を含みます．
サーバ内でオペレーション名が重複するなど，必要な場合には<service>ノードに
含まれるnsuri属性でサービスの名前空間を指定します．

<connection name='Sample1'>
	<Socket hostname='localhost' port='8765'/>
	<asynchronizedTTL>8000</asynchronizedTTL>
	<synchronizedTTL count="second">20</synchronizedTTL>
	<MaxProccessNumber>5</MaxProccessNumber>
</connection>
<connection name='Sample2'>
        <StdIO>
                <exec prog='/usr/local/sbin/SampleService' option='-s -u'/>
	</StdIO>
	<asynchronizedTTL>8000</asynchronizedTTL>
	<synchronizedTTL count="second">20</synchronizedTTL>
	<MaxProccessNumber>5</MaxProccessNumber>
</connection>

<connection name='Sample3'>
	<HTTP>
	  <url>http://services.opensoap.jp/cgi-bin/TargetService.cgi</url>
	</HTTP>
	<asynchronizedTTL>8000</asynchronizedTTL>
	<synchronizedTTL count="second">20</synchronizedTTL>
	<MaxProccessNumber>5</MaxProccessNumber>
</connection>

<connection>要素では，サービスとの接続方法を記述します．接続方法には

・socket:
	<Socket hostname='サービスプログラムのあるホスト名' port='ポート番号'/>
	（inetdもしくはxinetdへの登録が必要です．参照：RegistService.ujis）

・標準入出力:
	<StdIO>の子要素<exec prog='標準入出力で稼動するサービスプログラム本体の
	パス' option='プログラムに渡すコマンドライン引数'/>

・HTTP:
	<connection>の子要素として<url>を指定します。
	<url>には、呼出対象サービスのエンドポイントを指定します。

・名前つきパイプ:<FIFO>（未実装）
・IPC:<IPC>（未実装）
・COM:<COM>（未実装）
・その他接続モジュール:<Module>（未実装）
を指定することができます．

さらに，非同期プロセスでのタイムアウト時間(＝未処理のキュー、スプールの
データが破棄される時間)(秒)<asynchronizedTTL>，
同期プロセスでのタイムアウト処理<synchronizedTTL>（count属性で秒'second'，
またはメッセージのホップ回数'hoptimes'を指定できます．）を指定します．
これらは同時に指定することができ，リクエストメッセージによる同期・非同期の
指定によって使い分けられます．
  ホップ回数'hoptimes'での指定は、対象サービス呼出時に、当該サーバへの転送
回数をチェックし(received_pathの数をチェックする)、指定hoptimesより転送
回数の方が大きければ、サービス呼出しせずにFaultを発生する。


※同様の値として、server.conf中に以下の設定ができます。
  値の扱いとして、SSML中で有効な値の指定(0より大きい値。ただし、hoptimesに0を
  指定すると転送しない指定)があれば、server.confの値より優先して使用されます
  (値の大小によらず)。

-- in server.conf
  <limit>
    <ttl>
      <asynchronizedTTL>3600</asynchronizedTTL>
      <synchronizedTTL>600</synchronizedTTL>
      <hoptimes>4</hoptimes>
  </limit>
--
	
<MaxProccessNumber>で，この接続方法による最大接続数を指定できます．

<operation type ='Sample1'>add</operation>
<operation type ='Sample2'>sub</operation>

<operation>要素では，<connection>のname属性で指定した接続方法を指定して
オペレーション名を記述します．
ひとつのサービスに接続法の異なる複数のオペレーションがあってもかまいません．

<fault signature='1' />

<fault>要素ではサーバが返すFaultメッセージにサーバの署名を付加するかどうかを
signature属性('0' or '1')で指定します．

<MaxProccessNumber>15</MaxProccessNumber>

<MaxProccessNumber>でサービス全体のプロセス起動数を制限できます．

------
LastModified: Aug, 31. 2003

