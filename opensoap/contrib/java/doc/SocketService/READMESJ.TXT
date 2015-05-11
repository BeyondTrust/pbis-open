OpenSOAP For Java Service ReadMe

--------------------------------------------------------------------------------
[目次]
1.はじめに
2.サービス仕様
3.モジュール説明
4.コンパイル

--------------------------------------------------------------------------------
1.はじめに
     このプロジェクトを実行するには、Java2(JDK1.2以上)がインストールされている
     必要があります。
     また、このプロジェクトの実行にはSAX2.0対応のXMLパーサが必要です。
     （当プロジェクトではXercesを使用しています。)
--------------------------------------------------------------------------------
2.サービス仕様
     当サービスは、２項の計算処理を行います。またエラー(Fault)処理を行います。
     また、JavaサンプルSyncのSAXとの連携を確認しております。
     (処理手法などはモジュール説明を参考にしてください)
     サービス内容(下記内容を処理します。)
         Add       足し算
         Subtract  引き算
         Multiply  掛け算
         Divide    割り算
         Fault     faultメッセージを発生させます。
     起動方法
         java SocketService [PortNumber]

--------------------------------------------------------------------------------
3.モジュール説明
 3.1.OpenSoapConstants.java
     OpenSOAPサービスで使用する定数定義を行っています。
 3.2.OpenSoapException.java
     OpenSOAPサービスで使用するException定義を行っています。
 3.3.XMLWriter.java
     XML文章正規化処理を行っています。
 3.4.CalcServiceResponse.java
     受け取ったXMLをParse処理しています。
 3.5.SocketService.java
     指定されたポートをAccept待ちします。
     SOAPメッセージを受け取ります。

4.コンパイル
  javac [-classpath] OpenSoapConstants.java
  javac [-classpath] XMLWriter.java
  javac [-classpath] OpenSoapException.java
  javac [-classpath] CalcServiceResponse.java
  javac [-classpath] SocketService.java
　上記の順番で行ってください。

