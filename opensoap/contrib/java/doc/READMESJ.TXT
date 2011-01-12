OpenSOAP For Java Sample ReadMe

--------------------------------------------------------------------------------
[目次]
1.はじめに
2.必要モジュール
 2.1.ApacheSOAPを利用する場合
 2.2.saxを利用する場合
3.モジュール説明
 3.1.ApacheSOAPを利用する場合
  3.1.1.モジュール一覧
  3.1.2.
 3.2.saxを利用する場合
  3.1.1.モジュール一覧
4.その他
 4.1.構築バッチファイル
 4.2.実行バッチファイル

--------------------------------------------------------------------------------
1.はじめに
     このプロジェクトを実行するには、Java2(JDK1.2以上)がインストールされている
     必要があります。
     また、このプロジェクトの実行にはSAX2.0対応のXMLパーサが必要です。
     （当プロジェクトではXercesを使用しています。)
     その他必要モジュールに関しては「2.必要モジュール」を参照し、各実効環境に
     あったモジュールを適宜インストール願います。
--------------------------------------------------------------------------------
2.必要モジュール
 2.1.Java2SEのインストール
     http://www.java.sun.com/jse/1.3/ja/index.html
     http://www.java.sun.com/j2se/1.3/ja/install-linux-sdk.html
     よりファイルを取得しインストールします。
     下記環境変数を設定する。
       JAVA_HOMEにJavaSEへのPATHを（例：/usr/java/jdk1.3.1_01）追加します。
       PATHへ上記JAVA_HOME配下bin(例：/usr/java/jdk1.3.1_01/bin)を追加します。
     注意:WindowsNT/2000系はautoexec.batではなく、”マイコンピュータ”（右クリッ
          ク）”プロパティ””環境””ユーザ環境変数”にて行ってください。

 2.2.ApacheSOAPを利用する場合
      Apache Xerces XML Parcer 1.2.3 http://xml.apache.org/cerces-j
      Apache SOAP 2.0(2.2)           http://xml.apache.org/soap
      JavaMail(1.2)                  http://java.sun.com/products/javamail/
      JavaBeens(1.0.1) Activeation Framework
                          = http://java.sun.com/products/javabeans/glasgow/jaf.html
      jaf(http://java.sun.com/products/beans/glasgow/jaf.html)
          CLASSPATHにxerces.jar、soap.jar、mail.jar、activation.jarを追加

 2.2.saxを利用する場合
      Apache Xerces XML Parcer 1.2.3 http://xml.apache.org/cerces-j
--------------------------------------------------------------------------------
3.モジュール説明
 3.1.ApacheSOAPを利用する場合
  3.1.1.モジュール一覧
     CalcClient.java         メイン処理クラス
     CalcClientPanel.java    ユーザGUI設定・イベント処理クラス
     CalcClientRequest.java  SOAPメッセージ送受信処理クラス
   3.1.1.1. CalcClient.java
     GUIパネルを(CalcClientPanel.javaを利用して）用意します。
   3.1.1.2. CalcClientPanel.java
     ボタン押下によってイベントが発生するように設定。
     イベント発生によって(CalcClientRequest.javaを利用して）SOAPメッセージを
     送信します。
   3.1.1.3. CalcClientRequest.java
     サーバURLをGUIパネルより取得しGUIパラメータよりElementを作成し送信します。
     ApacheSOAP(SOAP.jar)を利用しinvokeします。
     注：下記メッセージ例を見ていただくとお分かりのように、Resultに関しては型の決まって
         いない状態です。soap.jarでは受け取った時点でNot Found DeserializerとなりException
         されてしまいます。そこで、このクラスには下記のようなマッピングをし回避しました。
        SOAPMappingRegistry smr = new SOAPMappingRegistry ();
        StringDeserializer sd = new StringDeserializer ();
        smr.mapTypes (Constants.NS_URI_SOAP_ENC,
                      new QName ("", "Result"), null, null, sd);

  3.1.2.メッセージ例
   3.1.2.1.リクエスト
     POST /calc HTTP/1.0
     Host: localhost
     Content-Type: text/xml; charset=utf-8
     Content-Length: 478
     SOAPAction: "http://services.opensoap.jp/samples/Calc/"

     <?xml version='1.0' encoding='UTF-8'?>
     <SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/"
                        xmlns:xsi="http://www.w3.org/1999/XMLSchema-instance"
                        xmlns:xsd="http://www.w3.org/1999/XMLSchema">
       <SOAP-ENV:Body>
         <ns1:Add xmlns:ns1="http://services.opensoap.jp/samples/Calc/"
                  SOAP-ENV:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
           <A xsi:type="xsd:int">2</A>
           <B xsi:type="xsd:int">5</B>
         </ns1:Add>
       </SOAP-ENV:Body>
     </SOAP-ENV:Envelope>

   3.1.2.2.レスポンス
     <?xml version="1.0" encoding="utf-8"?>
     <SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/">
       <SOAP-ENV:Body>
         <m:AddResponse xmlns:m="http://services.opensoap.jp/samples/Calc/">
           <Result>2.000000</Result>
         </m:AddResponse>
       </SOAP-ENV:Body>
     </SOAP-ENV:Envelope>

   3.1.2.3.faultメッセージ
     HTTP/1.1 200 OK
     Content-Type: text/xml; charset="utf-8"
     Content-Length: 388

     <?xml version="1.0"?>
     <SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/"
                        SOAP-ENV:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
       <SOAP-ENV:Body>
         <SOAP-ENV:Fault>
           <faultcode>OPENSOAP:ffffffff</faultcode>
           <faultstring>OPENSOAP Service error</faultstring>
           <detail>OPENSOAP Service Internal error</detail>
         </SOAP-ENV:Fault>
       </SOAP-ENV:Body>
     </SOAP-ENV:Envelope>

 3.2.saxを利用する場合
  3.1.1.モジュール一覧
     CalcClient.java         メイン処理クラス
     CalcClientPanel.java    ユーザGUI設定・イベント処理クラス
     CalcClientRequest.java  Calcサービスメッセージ処理クラス
     OpenSoapConstants.java  OpenSOAPとしての定数定義。
     OpenSoapEnvelope.java   SOAPメッセージEnvelope処理クラス
     OpenSoapRequest.java    基本SOAPメッセージ処理クラス
     OpenSoapException.java  OpenSOAPエラー時処理
     XMLWriter.java          XML文章正規化処理クラス
   3.1.1.1. OpenSoapConstants.java
     当プロジェクト全体で使用される定数を集めたクラスです。
   3.1.1.2. XMLWriter.java
     予約文字（'<''&')などをエスケープ処理するクラスです。
   3.1.1.3. OpenSoapException.java
     faultcode/faultstringを管理するクラスです。
   3.1.1.4. OpenSoapEnvelope.java
     エンベロープをパースするクラスです。
     XMLFilterImplを継承しています。
   3.1.1.5. OpenSoapRequest.java
     リクエスト用にSOAPプロトコルを実装しています。
     リクエストを準備し、HTTPより送信しOpenSoapEnvelopeを使用しレスポンスを
     デコードしています。
   3.1.1.6. CalcClientRequest.java
     OpenSoapRequestを継承しています。
   3.1.1.7. CalcClientPanel.java
     クライアント側のユーザインタフェースをサポートしているクラスです。
   3.1.1.8. CalcClient.java
     クライアント側のメインクラスです。

  3.1.2.メッセージ例
   3.1.2.1.リクエスト
     POST /calc HTTP/1.1
     Content-Length: 306
     Content-Type: text/xml
     SOAPAction: ""http://services.opensoap.jp/samples/Calc/""
     User-Agent: Java1.3.0
     Host: localhost:8070
     Accept: text/html, image/gif, image/jpeg, *; q=.2, */*; q=.2
     Connection: keep-alive

     <?xml version='1.0'?>
     <SOAP-ENV:Envelope xmlns:SOAP-ENV='http://schemas.xmlsoap.org/soap/envelope/'>
       <SOAP-ENV:Body>
         <ns1:Add xmlns:ns1='http://services.opensoap.jp/samples/Calc/'
                  SOAP-ENV:encodingStyle='http://schemas.xmlsoap.org/soap/encoding/'>
           <A>1</A>
           <B>1</B>
         </ns1:Add>
       </SOAP-ENV:Body>
     </SOAP-ENV:Envelope>

   3.1.2.2.レスポンス
     <?xml version="1.0" encoding="UTF-8"?>
     <SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/">
       <SOAP-ENV:Body>
         <m:AddResponse xmlns:m="http://services.opensoap.jp/samples/Calc/">
           <Result>2.000000</Result>
         </m:AddResponse>
       </SOAP-ENV:Body>
     </SOAP-ENV:Envelope>

   3.1.2.3.faultメッセージ
     HTTP/1.1 200 OK
     Content-Type: text/xml
     Content-Length: 388

     <?xml version="1.0"?>
     <SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/"
                        SOAP-ENV:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
       <SOAP-ENV:Body>
         <SOAP-ENV:Fault>
           <faultcode>OPENSOAP:ffffffff</faultcode>
           <faultstring>OPENSOAP Service error</faultstring>
           <detail>OPENSOAP Service Internal error</detail>
         </SOAP-ENV:Fault>
       </SOAP-ENV:Body>
     </SOAP-ENV:Envelope>

 3.3.クライアントユーザインタフェース
     --------------------------------------------------
     | Server          |  (1)               |         |
     --------------------------------------------------
     |   |(2)|      |(3)|▼|     |(4)|  (5)   (6)     |
     --------------------------------------------------
     | (7)                                            |
     --------------------------------------------------
     (1) サーバURIの入力域
         入力は、(サーバURL):(ポート番号)とします。
         たとえば、サーバのURL(ここではIPとします)が133.50.1.90
         サービスポート番号が8764の場合、http://133.50.1.90:8764
         とします。
     (2)(4) 数値１・２
         計算対象の数値を入力します。
     (3) 計算方式の選択
         Add       足し算
         Subtract  引き算
         Multiply  掛け算
         Divide    割り算
         Fault     faultメッセージを発生させます。
         を数値１・２に対し行います。
     (5) 実行ボタン
         押下によってSOAPメッセージが送信されレスポンスが帰ってきます。
     (6) 結果表示（計算結果）
         計算（サービス）結果を表示します。
         この場合答えを表示します。
     (7) 結果表示（処理結果）
         Checking:リクエスト処理・レスポンス処理中
         空白：正常処理中
--------------------------------------------------------------------------------
4.その他
 4.1.構築バッチファイル
  4.1.1.ApacheSOAPを利用する場合
        (Win)ApacheSOAPBuild.batを使用します。
        ファイル内の説明に従い使用環境に合わせて変更してください。
  4.1.2.saxを利用する場合
        (Win)SAXClientBuild.batを使用します。
        ファイル内の説明に従い使用環境に合わせて変更してください。
 4.2.実行バッチファイル
  4.1.1.ApacheSOAPを利用する場合
        (Win)ApacheSOAPExec.batを使用します。
        (Unix)ApacheSOAPExec.shを使用します。
  4.1.2.saxを利用する場合
        (Win)SaxClientExec.batを使用します。
        (Unix)SaxClientExec.shを使用します。
  4.1.3.ApacheSOAP付属トンネルモニタ
        (Win)Tunnel.batを使用します。
        (Unix)Tunnel.shを使用します。
        ApacheSOAPをインストールすると付属しているTcpトンネルモニタを起動します。
        通常実行には下記のように入力します。
        >java org.apache.soap.util.net.TcpTunnelGui 8070 localhost 8080
        この記述をバッチファイル化しただけのものです。
        Portは8070を8080へトンネルしますので使用環境などへの適応をしてください。

補足：バッチファイル実行時に「環境変数のための領域が足りません」と表示される(DOS)
        原因は環境変数のためのメモリが足りないからです。
        以下のようにして環境変数を増やしてあげましょう。
        (1)DOS窓を開く
        (2)ALTとスペースキーの同時押下にてプルダウンメニューを表示します。
        (3)プロパティを選択します。
        (4)MS-DOSプロンプトのプロパティパネル表示
        (5)メモリタグを選択
        (6)環境変数の初期サイズ(V)のプルダウンメニューで大きな値を選択
        (7)OKボタンを押下する。
        (8)一度DOS窓を閉じ、再度起動させます。

