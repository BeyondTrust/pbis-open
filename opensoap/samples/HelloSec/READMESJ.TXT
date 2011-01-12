HelloSec サンプル
=================


[1] 概要: 

  このディレクトリには、OpenSOAP のセキュリティ機能の使用方法および実行例を
  示すことを目的としたシンプルな Web サービスのサンプルプログラムである 
  HelloSec サンプルのサービスおよびクライアントプログラム一式が含まれています。
  
  HelloSec Web サービスは、Hello メソッドの MyName パラメータで指定された
  文字列を受け取り、"Hello (文字列)!"を Reply パラメータとして返します。
  
  リクエストおよびレスポンスメッセージは、パラメータが暗号化された後、
  電子署名が付加されて送信されます。
  
  受信されたリクエストおよびレスポンスメッセージは、署名が検証された後、
  パラメータが復号化されてから処理されます。
  
  実際に送受信されるリクエスト／レスポンスメッセージの例は、
  hello_sec_request.xml および、hello_sec_response.xml を参照してください。
  
  暗号化・復号化および署名付加・検証に使用される鍵ファイルは以下のとおりです。
  
  鍵ファイル名              意味
  ------------------------  --------------------
  privKey_HelloClient.pem   クライアントの秘密鍵
  pubKey_HelloService.pem   サービスの公開鍵    
  pubKey_HelloClient.pem    クライアントの秘密鍵
  privKey_HelloService.pem  サービスの公開鍵    

  各鍵ファイルの具体的な使われ方は以下のとおりです。

  使用ルーチン              プログラム     目的      鍵ファイル名
  ------------------------  -------------  --------  ------------------------
  			    	        
  リクエストメッセージ作成  クライアント   暗号化    pubKey_HelloService.pem
                                           署名付加  privKey_HelloClient.pem
  			    	        
  リクエストメッセージ解析  サービス       署名検証  pubKey_HelloClient.pem
                                           復号化    privKey_HelloService.pem
			    	        
  レスポンスメッセージ作成  サービス       暗号化    pubKey_HelloClient.pem
                                           署名付加  privKey_HelloService.pem
			    	        
  レスポンスメッセージ解析  クライアント   署名検証  pubKey_HelloService.pem
                                           復号化    privKey_HelloClient.pem


[2] 実行ファイルの作成:

  OpenSOAP API インストール後は、make clean したあとで、make -f HelloSec.mak 
  を行って下さい。以下のファイルが作成されます。
  
  * HelloSecClient
    HelloSec クライアントプログラムです。
  
  * HelloSecService
    標準入出力を用いた HelloSec サービスプログラムです。
  
  * HelloSecService.cgi
    CGI 型の HelloSec サービスプログラムです。cgi が実行可能なディレクトリに
    置くことで利用可能となります。


[3] インストール:

  プログラムファイル、設定ファイル、データファイル等は適切な場所に
  インストールする必用があります。
  
  多くの実行環境では、make -f HelloSec.mak install で適切な場所に
  インストールされます。
  
  ただし、cgi 版プログラムをインストールする場所は、make ファイルの
  CGI_BIN_DIR 変数に設定しておく必用があります。
  
  HelloSec.mak でインストールされる各ファイルとインストール場所は
  以下のとおりです。
  
  インストールされるファイル  インストール場所
  --------------------------  ---------------------------
  HelloSecService.cgi         /home/httpd/cgi-bin
  HelloSecService             /usr/local/opensoap/services
  HelloSec.ssml               /usr/local/opensoap/conf/ssml
  privKey_HelloService.pem    /usr/local/opensoap/services/HelloSec
  pubKey_HelloClient.pem      /usr/local/opensoap/services/HelloSec
  
  詳しくは HelloSec.mak ファイルを参照してください。


[4] 実行:
  
  サンプルの実行の仕方等は、HelloSec.mak ファイルを参照してください。
  ファイルの後部に実行例としてテスト実行用のルールが記述してあります。
  
  たとえば、ルール ctest の場合、以下のように実行することができます。
  
  make -f HelloSec.mak ctest
  
  テスト実行用の各ルールの意味は以下のとおりです。
  
  * stest (サービス単体テスト)
    予め作成されたリクエストメッセージ(テキストファイル)を CGI 版のサービスに
    入力してサービスプログラムの単体テストを行います。
    
  * ctest (クライアント・サービス連携テスト)
    クライアントから CGI 版のサービスを呼び出すテストを行います。
    
  * cstest (クライアント・サーバ・サービス連携テスト)
    クライアントから OpensOAP サーバを経由して標準入出力版のサービスを呼び出す
    テストを行います。
    実行にするためには、opensoap-server-ctl start を実行して OpenSOAP サーバを
    起動しておく必要があります。


