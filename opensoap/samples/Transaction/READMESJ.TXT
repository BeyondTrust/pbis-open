OpenSOAP Transaction Service Sampleドキュメント

2003/1/31

トランザクション管理サービス本体は，src/tools/Transaction/にあります．

トランザクションサービスに関するドキュメントは，doc/tools/Transaction/
にあります．
本サンプルサービスを利用してOpenSOAPトランザクションの詳細を説明してい
ます．

このディレクトリには，以下に示す，OpenSOAP Transactionを利用するための
サンプルクライアントおよびサービスのソースコードが含まれています。
サンプルサービスは，仮想的な銀行の入出金サービスです．

* TransactionClient
　銀行振り込みサービスを利用するクライアントプログラムです．
　コマンドラインで使用します．

　コマンドの使い方：
　TransactionClient [-s endpoint] [-a soapaction] [-c char_enc]
　method(amount) [account(from)] [account(to)] [amount]
  　-s endpoint    SOAP service endpoint URI
  　-a soapaction  value of SOAPAction HTTP-Header
  　-c char_enc    character encoding of request (default: UTF-8)
  　method         Sync, Async or GetResult.
  　account(from)  payment from (except for GetResult)
  　account(to)    deposit to (except for GetResult)
  　amount         amount of money to transfer (except for GetResult)

* TransactionApp
　CGIを利用したGUIクライアントプログラムです．

* TransactionABankService
　入金メソッド，出金メソッドを受け付ける，OpenSOAPトランザクションに対
　応した仮想銀行サービスプログラムです．
　定数定義しているアカウントの口座へのアクセス以外はエラーを返します．

　リクエストメソッド
　PaymentRequest：出金要求を行います．アカウントと金額を子要素とします．
　DepositRequest：入金要求を行います．アカウントと金額を子要素とします．
　TransactionAction：コミット，ロールバックを受け付けます．
