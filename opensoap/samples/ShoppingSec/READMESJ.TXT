ショッピングサンプル
====================


[1] 概要: 

  このディレクトリには、単純化した B2C-EC を SOAP で行うショッピング Web 
  サービスのクライアントおよびサービスのサンプルコードが含まれています。

  ショッピング Web サービスは、以下の 4 つのメソッドから構成されます。

  メソッド名       意味          パラメータ1   パラメータ2   返り値
  ---------------  ------------  ------------  ------------  --------------
  GetProductCount  商品種数取得  (なし)        (なし)        商品種数
  GetProductSpec   商品明細取得  インデックス  (なし)        商品明細
  GetStockQty      在庫数取得    商品コード    (なし)        商品在庫数
  PlaceOrder       発注          商品コード    個数          発注成功／失敗

  各メソッドのリクエスト／レスポンスメッセージの例は、SampleMessages
  ディレクトリの各ファイルを参照してください。

  クライアントプログラムでは、GetProductCount と GetProductSpec を組み
  合わせた GetProductList というメソッドで商品リストの取得を行っています。


[2] 実行ファイルの作成:

  OpenSOAP API インストール後、2つのいずれかの make ファイルを用いて、
  make -f ファイル名 で、それぞれ以下の実行ファイルが作成されます。
  
  (1) Shopping.mak  --- 同期・非同期版
      (PlaceOrder のみ同期か非同期で実行できます)

    * ShoppingService
      標準入出力を用いたショッピングサービスプログラムです。

    * ShoppingService.cgi
      CGI 型のショッピングサービスプログラムです。

    * ShoppingClient
      同期版ショッピングクライアントプログラムです。

    * ShoppingClientAsync
      PlaceOrderのみ非同期処理対応のショッピングクライアントプログラム。
	  OpenSOAPサーバ経由でサービスを呼び出す時に非同期処理を実行できます。

  (2) ShoppingSec.mak   --- セキュリティ版
      (PlaceOrder のみ暗号化・署名付加して実行します)

    * ShoppingServiceSec
      標準入出力を用いたセキュリティ版ショッピングサービスプログラムです。

    * ShoppingServiceSec.cgi
      CGI 型のセキュリティ版ショッピングサービスプログラムです。

    * ShoppingClientSec
      セキュリティ版ショッピングクライアントプログラムです。

    * ShoppingClientSec.cgi
      CGI 型のセキュリティ版ショッピングクライアントプログラムです。

  
[3] インストール:
  
  プログラムファイル、設定ファイル、データファイル等は適切な場所に
  インストールする必用があります。
  
  多くの実行環境では、make -f ファイル名 install で適切な場所に
  インストールされます。
  
  詳しくはそれぞれの make ファイルを参照してください。
  
  
[4] 実行:

  サンプルの実行の仕方等は、それぞれの make ファイルを参照してください。
  make ファイルの後部に実行例がルールとして記述してあります。

  make -f Shopping.mak *test*

    サンプルリクエストメッセージを使ったサービスのテスト
    (ルール)(説明)         (使用サービスメソッド名)
    stest1  商品種数取得   GetProductCount
    stest2  商品明細取得   GetProductSpec
    stest3  在庫数取得     GetStockQty
    stest4  発注           PlaceOrder

    stest5  商品種数取得   GetProductCount (失敗例)
    stest6  商品明細取得   GetProductSpec  (失敗例)
    stest7  在庫数取得     GetStockQty     (失敗例)
    stest8  発注           PlaceOrder      (失敗例)

    クライアントのテスト
    ctest1 全商品明細取得  GetProductCount + GetProductSpec
    ctest2 在庫数取得      GetStockQty
    ctest3 発注            PlaceOrder
    ctest1a  (ctest1の非同期版クライアントShoppingClientAsyncでのテスト)
    ctest2a  (ctest2の 〃 )
    ctest3a  (ctest3の 〃 )

    クライアントのテスト(OpenSOAPサーバ経由)
    cstest1 ShoppingClientで全商品明細取得 GetProductCount + GetProductSpec
    cstest2 ShoppingClientで在庫数取得     GetStockQty
    cstest3 ShoppingClientで同期処理での発注 PlaceOrder

    cstest1a ShoppingClientAsyncで全商品明細取得 GetProductCount + GetProductSpec
            （ただし同期なのでcstest1と同じ処理）
    cstest2a ShoppingClientAsyncで在庫数取得     GetStockQty
            （ただし同期なのでcstest2と同じ処理）
    cstest3a ShoppingClientAsyncで同期処理での発注 PlaceOrder
    cstest4a ShoppingClientAsyncで非同期処理での発注 PlaceOrderAsync
    cstest5a cstest4aの処理結果の取得    ConfirmOrder

  make -f ShoppingSec.mak *test*

    サンプルリクエストメッセージを使ったサービスのテスト
    stest1  商品種数取得   GetProductCount
    stest2  商品明細取得   GetProductSpec
    stest3  在庫数取得     GetStockQty
    stest4  発注           PlaceOrder

    クライアントのテスト
    ctest1 全商品明細取得  GetProductCount + GetProductSpec
    ctest2 在庫数取得      GetStockQty
    ctest3 発注            PlaceOrder
  
    クライアントのテスト(OpenSOAPサーバ経由)
    cstest1 全商品明細取得 GetProductCount + GetProductSpec
    cstest2 在庫数取得     GetStockQty
    cstest3 発注           PlaceOrder     

  セキュリティ版のみ、Web アプリケーション版クライアント
  ShoppingClientSec.cgi があり、Web ブラウザで実行できます。

  Web ブラウザで入力する URL は、たとえば以下のようになります。

  http://localhost/cgi-bin/ShoppingClientSec.cgi
