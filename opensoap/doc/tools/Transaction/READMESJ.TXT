OpenSOAP Transaction Service ドキュメント
2002/2/28
トランザクション管理サービス本体は，src/tools/Transaction/にあります．
Clientと入出金サービスのサンプルはsamples/Transaction/にあります．

１．概要
OpenSOAP Transaction Service（以下，トランザクションサービス）は，以下
の機能を実現します．
・仕様にしたがって複数の要求メッセージがそれぞれSOAP Bodyblockとして挿
  入されたSOAPメッセージ(SOAP Envelope)をクライアントから受け取る(例１)
  と，それぞれを要求メッセージとしてEnvelopeを分割生成(例２)し，指定さ
  れた各URL(endpoint)に送信します．
・トランザクションサービスに対応した各サービスから，仕様にしたがって返
  答メッセージを受け取ると，各サービスの処理の成功・失敗に応じて，
  COMMIT要求，もしくはROLLBACK要求を再送します．
・各サービスからのレスポンスメッセージと，最終的な処理結果（COMMITまた
  はROLLBACK）をクライアントに返送します．
・OpenSOAPサーバと連携することで，クライアントは非同期的に最終メッセー
  ジを受け取ることが出来ます．

トランザクションサービスを利用するためには，クライアントと，トランザク
ションサービスから接続される各サービスがトランザクションサービスに対応
している必要があります．


２．クライアント仕様（リクエストメッセージ）
クライアントは以下の仕様に従ったリクエストメッセージを送信できる必要が
あります．

2.1.トランザクション制御記述ブロック
リクエストメッセージのBody（<SOAP-ENV:Body>）に含まれる最初のBodyblock
は<TransactionControl>である必要があります(例１)．<TransactionControl>
ブロックのnamespaceは"http://services.opensoap.jp/transaction/"である
ことが望ましいです．
<TransactionControl>ブロックには，トランザクション処理の対象となるサー
ビスの数だけ，子要素<endpoint>が，要求される処理の順番に並んでいる必要
があります．
<endpoint>要素には各サービスがトランザクション処理として実行可能なSOAP
メッセージの送信先URLを記述します．また，各要求処理ごとに固有の任意の
属性値"transactionRequestID"を付加します．
その他，トランザクション処理に関わる処理設定はこのブロックに記述されま
す．（β版での実装は<endpoint>要素のみ）

2.2.トランザクションヘッダブロック
各サービスに送信されるべきメッセージのヘッダ部が存在する場合，トランザ
クションヘッダブロック<TransactionHeaderBlock>の要素として記述します
(例１)．この部分に記述された要素は，各サービスに送信されるSOAPメッセー
ジのヘッダブロック（<SOAP-ENV:Header>の直下のブロック）として挿入され
ます．<TransactionHeaderBlock>ブロックのnamespaceは
"http://services.opensoap.jp/transaction/"であることが望ましいです．

各<TransactionHeaderBlock>ブロックには，<TransactionControl>ブロックの
<endpoint>要素の対応する属性値"transactionRequestID"を付加する必要があ
ります．<TransactionHeaderBlock>ブロックは第２Bodyblock以降であれば，
どの順番で挿入されていてもかまいません．

2.3.トランザクションボディブロック
各サービスに送信されるべきメッセージのボディ部は，トランザクションボディ
ブロック<TransactionBodyBlock>の要素として記述します(例１)．この部分に
記述された要素は，各サービスに送信されるSOAPメッセージのボディブロック
（<SOAP-ENV:Body>の直下のブロック）として挿入されます．
<TransactionBodyBlock>ブロックのnamespaceは
"http://services.opensoap.jp/transaction/"であることが望ましいです．

各<TransactionBodyBlock>ブロックには，<TransactionControl>ブロックの
<endpoint>要素の対応する属性値"transactionRequestID"を付加する必要があ
ります．<TransactionBodyBlock>ブロックも，<TransactionHeaderBlock>ブロッ
ク同様，第２Bodyblock以降であれば，どの順番で挿入されていてもかまいま
せん．


例１. OpenSOAP Transaction Request Envelope
A銀行に出金の要求をし，それが受け入れられたらB銀行に同額の入金要求をす
るトランザクションの例．

<SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/" SOAP-ENV:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
<SOAP-ENV:Body>
	<t:TransactionControl xmlns:t="http://services.opensoap.jp/transaction/">
		<endpoint transactionRequestID="1">http://opensoap.jp/cgi-bin/soapInterface.cgi</endpoint>
		<endpoint transactionRequestID="2">http://opensoap.jp/cgi-bin/b_bank_transfer.cgi</endpoint>
	</t:TransactionControl>
	<t:TransactionBodyBlock xmlns:t="http://services.opensoap.jp/transaction/" transactionRequestID="1">
		<p:PaymentRequest xmlns:p="http://services.opensoap.jp/a_bank_transfer/">
			<account>1338675</account>
			<amount>$5000</amount>
			<to>b_bank:1252412</to>
		</p:PaymentRequest>
	</t:TransactionBodyBlock>
	<t:TransactionHeaderBlock transactionRequestID="2" xmlns:tp="http://services.opensoap.jp/transaction/">
		<e:encrypt xmlns:e="http://security.opensoap.jp/1.0/" api="OpenSOAP">true</e:encrypt>
	</t:TransactionHeaderBlock>
	<t:TransactionBodyBlock xmlns:t="http://services.opensoap.jp/transaction/" transactionRequestID="2">
		<d:DepositRequest xmlns:d="http://services.opensoap.jp/b_bank_transfer/">
			<account>1252412</account>
			<amount>$5000</amount>
			<from>a_bank:1338675<from>
		</d:DepositRequest>
	</t:TransactionBodyBlock>
</SOAP-ENV:Body>
</SOAP-ENV:Envelope>


３．トランザクション対応サービス仕様
OpenSOAP Transaction Serviceに対応したサービスは，クライアントが生成す
るSOAPメッセージの仕様（ヘッダブロック，ボディブロックの内容）とサービ
スの機能をクライアントに公開すると供に，トランザクションサービスとの接
続について，以下の仕様に従う必要があります．

3.1.要求処理に対する返信
トランザクションサービスによって分割生成された各Envelope(例２)は，対応
するそれぞれのendpointに送信されます．各サービスは，Envelopeにしたがっ
て要求された処理を行い，その処理をRollbackできる状態で保存した後，レス
ポンスメッセージ(例３)をトランザクションサービスに返信します．レスポン
スメッセージの仕様は各サービスごと独自でかまいませんが，第1ボディブロッ
クの直下には処理の結果を記述する子要素<TransactionResult>を含む必要が
あります．
<TransactionResult>には，そのサービスでの処理の成功を通知する"SUCCESS"
を記述します．それ以外の文字列，もしくはサービスから<SOAP-ENV:Fault>等
が返信された場合は，そのサービスでの処理は失敗，もしくは拒否されたもの
とみなされます．
また，<TransactionResult>には，サービス内で固有の識別IDを属性値
"transactionID"として付加することができます．"transactionID"属性を使う
と，次のステップでトランザクションサービスからCOMMITまたはROLLBACKの要
求がなされた時に，サービス内で整合をとることができます．

3.2.COMMITまたはROLLBACK要求
トランザクションサービスはサービスから成功（"SUCCESS"）が返信されると，
<TransactionControl>ブロックに記述された順番に従って次のサービスへのリ
クエストメッセージを送信します．
全てのサービスからSUCCESSが返信された場合，トランザクションサービスは
再び全てのendpointに対して対応する"transactionID"属性値を付加した
COMMIT要求メッセージ(例４)を送信して一連の処理の実行を決定します．
全てのサービスからSUCCESSが返信される前に，途中のサービスから失敗メッ
セージが返信されると，その前までにSUCCESSが返信されたサービス群に対し
て"transactionID"属性値を付加したROLLBACK要求メッセージを送信します．

3.3.処理の完結
COMMIT要求を受け取ったサービスは要求された処理を実行して終了し，
ROLLBACK要求を受け取ったサービスは対応する処理を取り消します．さらに，
トランザクションサービスにそれぞれの処理が完了した旨のSOAPメッセージ
(例５)を返信します．


例２(A)．A銀行へ送信されるSOAP Envelope

<?xml version="1.0" encoding="UTF-8"?>
<SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/">
<SOAP-ENV:Body>
<p:PaymentRequest xmlns:p="http://services.opensoap.jp/a_bank_transfer/">
	<account>1338675</account>
	<amount>$5000</amount>
	<to>b_bank:1252412</to>
</p:PaymentRequest>
</SOAP-ENV:Body>
</SOAP-ENV:Envelope>

例２(B)．B銀行へ送信されるSOAP Envelope

<?xml version="1.0" encoding="UTF-8"?>
<SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/">
<SOAP-ENV:Body>
<d:DepositRequest xmlns:d="http://services.opensoap.jp/b_bank_transfer/">
	<account>1252412</account>
	<amount>$5000</amount>
	<from>a_bank:1338675</from>
</d:DepositRequest>
</SOAP-ENV:Body>
</SOAP-ENV:Envelope>

例３．A銀行からのレスポンスの例

<?xml version="1.0" encoding="UTF-8"?>
<SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/">
<SOAP-ENV:Body>
<p:PaymentResponse xmlns:p="http://services.opensoap.jp/samples/a_bank_transfer/">
	<t:TransactionResult xmlns:t="http://services.opensoap.jp/transaction/" transactionID="abank13386751012192663">
	SUCCESS
	</t:TransactionResult>
	<p:Comment>1338675 sent $5000 to b_bank:1252412</p:Comment>
</p:PaymentResponse>
</SOAP-ENV:Body>
</SOAP-ENV:Envelope>


例４．トランザクションサービスからA銀行へのCOMMIT要求

<?xml version="1.0" encoding="UTF-8"?>
<SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/">
<SOAP-ENV:Body>
	<t:TransactionAction xmlns:t="http://services.opensoap.jp/transaction/" transactionID="abank13386751012192663">
	COMMIT
	</t:TransactionAction>
</SOAP-ENV:Body>
</SOAP-ENV:Envelope>


例５．A銀行からCOMMIT要求に対する返信

<?xml version="1.0" encoding="UTF-8"?>
<SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/">
<SOAP-ENV:Body>
	<t:TransactionActionResponse xmlns:t="http://services.opensoap.jp/transaction/" transactionID="abank13386751012192663">
	COMMITED
	</t:TransactionActionResponse>
</SOAP-ENV:Body>
</SOAP-ENV:Envelope>

４．クライアントへの返信
トランザクションサービスは，一連の処理の結果をリクエストメッセージと同
様にひとつのEnvelopeに結合し(例６），クライアントに返信します．SOAPメッ
セージの最初のボディブロック<TransactionResponse>ブロックには，トラン
ザクションの最終結果が，COMMITまたはROLLBACKとして記述されます．２つめ
以降のボディブロックは，各サービスから返信されたレスポンスメッセージの
ヘッダブロックが<TransactionHeaderBlock>ブロックに，ボディブロックが
<TransactionBodyBlock>ブロックに，さらにCOMMITもしくはROLLBACKを送信し
た際のレスポンスメッセージのボディブロックが
<TransactionActionResponseBodyBlock>に，それぞれリクエストメッセージに
付加した属性値"transactionRequestID"と同じ属性値が付加されて挿入されま
す．


例６．クライアントに返信されるトランザクション処理結果

<?xml version="1.0" encoding="UTF-8"?>
<SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/">
<SOAP-ENV:Body>
	<t:TransactionResponse xmlns:t="http://services.opensoap.jp/transaction/">
	COMMIT
	</t:TransactionResponse>
	<t:TransactionBodyBlock xmlns:t="http://services.opensoap.jp/transaction/" transactionRequestID="1">
		<p:PaymentResponse xmlns:p="http://services.opensoap.jp/samples/a_bank_transfer/">
			<t:TransactionResult transactionID="pt13386751012192663">
				SUCCESS
			</t:TransactionResult>
			<:Comment xmlns:="">
				1338675 sent $5000 to b_bank:1252412
			</:Comment>
		</p:PaymentResponse>
	</t:TransactionBodyBlock>
	<t:TransactionActionResponseBodyBlock xmlns:t="http://services.opensoap.jp/transaction/" transactionRequestID="1">
		<t:TransactionActionResponse transactionID="pt13386751012192663">
			COMMITED
		</t:TransactionActionResponse>
	</t:TransactionActionResponseBodyBlock>
	<t:TransactionBodyBlock xmlns:t="http://services.opensoap.jp/transaction/" transactionRequestID="2">
		<p:DepositResponse xmlns:p="http://services.opensoap.jp/samples/a_bank_transfer/">
			<t:TransactionResult transactionID="dt12524121012226863">
				SUCCESS
			</t:TransactionResult>
			<p:Comment>
				1252412 received $5000 from a_bank:1338675
			</p:Comment>
		</p:DepositResponse>
	</t:TransactionBodyBlock>
	<t:TransactionActionResponseBodyBlock xmlns:t="http://services.opensoap.jp/transaction/" transactionRequestID="2">
		<t:TransactionActionResponse transactionID="dt12524121012226863">
			COMMITED
		</t:TransactionActionResponse>
	</t:TransactionActionResponseBodyBlock>
</SOAP-ENV:Body>
</SOAP-ENV:Envelope>
