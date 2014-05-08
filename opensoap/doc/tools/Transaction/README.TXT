The OpenSOAP Transaction Service

The transaction management service is located in src/tools/Transaction/ .
A client money transfer service sample is located in sample/Transaction/ .

1. Overview
The OpenSOAP Transaction Service (Transaction Service) provides the
following functionality.
 - In accordance with the specification, several request messages in
   their respective SOAP Bodyblocks contained in the SOAP message(SOAP
   Envelope) are received from the client(Ex. 1). These request
   messages are used to generate their respective Envelopes(Ex. 2) and
   are sent to their specified URLs(endpoint).
 - The transaction service receives a response message in accordance
   with the specification from each of its corresponding services, and
   based on the success or failure of the service process resends a
   COMMIT request or ROLLBACK request.
 - The response message from each service and the overall process
   result (COMMIT or ROLLBACK) is returned to the client.
 - Thus, when interacting with an OpenSOAP Server, the client is able
   to receive the final response message asynchronously.

In order to use the transaction service, the client and the services
connected to the transaction service, must be compatible with the
transaction service.


2. Client Specification(Request Message)
The Client must be able to send a request message in accordance with
the specifications below.

2.1. Transaction Control Description Block
The <TransactionControl> must be the first Bodyblock(Ex. 1) contained
in the request message Body (<SOAP-ENV:Body>).
It is desireable to have "http://services.opensoap.jp/transaction/" in
the namespace of the <TransactionControl> block.
The <TransactionControl> block contains the exact number of services
relevant to the transaction processing, and it is required to list
these <endpoint> sub-elements in the order required for processing
these requests.
The <endpoint> element, for each service, contains the URL to which is
sent the SOAP message used for the transaction process.
Also, a characteristic optional attribute value "transactionRequestID"
is added to each request process.

Also, the process settings having to do with the transaction
processing are described in this block.
(In the Beta Version, only the <endpoint> element is implemented)

2.2. Transaction Header Block
If message header part that is to be sent to each service exists, then
the transaction header block <TransactionHeaderBlock> elements can be
defined(Ex. 1). The elements defined in this part are included as the
SOAP message header block (immediately below the <SOAP-ENV:Header>
block) to be sent to each service. It is desired that namespace of the
<TranscationHeaderBlock> be set to
"http://services.opensoap.jp/transaction/".
Each <TransactionHeaderBlock> block is required to have a
corresponding "transactionRequestID" attribute value of the
<TransactionControl> block <endpoint> element added to it. For the 2nd
and successive Bodyblocks of the <TransactionHeaderBlock>, the
ordering is irrelevant.

2.3. Transaction Body Block
The message body part that should be sent to each of the services, is
defined as transaction body block <TransactionBodyBlock> element. The
elements defined in this part are inserted as SOAP message body blocks
(immediately below the <SOAP-ENV:Body> block)to be sent to each
service.
It is desired that namespace of the <TransactionBodyBlock> be set to
"http://services.opensoap.jp/transaction/".
It is required that corresponding <endpoint> "transactionRequestID"
attribute value of the <TransactionControl> block be added to each
<TransactionBodyBlock>. As for the <TransactionHeaderBlock>, in the
<TransactionBodyBlock>, 2nd and successive Bodyblock ordering is
irrelevant.


Ex.1. OpenSOAP transaction Request Envelope
A transaction example of a withdrawl request from Bank A and a
lodgement request to Bank B for the same amount.

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


3. Transaction Response Service Specification
The services relevant to the OpenSOAP Transaction Service, the
specification for the client generated SOAP message (Header Block,
Body Block Content), and the service functions made available to the
client along with connecting to the transaction service must comply
with the specification listed below.


3.1. Request Processing Response
Every Envelope, generated seperately according to the transaction
service(Ex. 2) is sent to its corresponding endpoint.
Every service performs its request processing according to the
Envelope, saves the process status so a Rollback is possible, and
returns a response message(Ex. 3) to the transaction service.
The specification of the response message from every service can be
independent, however, a process result description element
<TranscationResult> must be included immediately below the first Body
Block.
The <TransactionResult>, conveys the success of that services and is
defined as "SUCCESS".
Any other character sequence, or the return of a <SOAP-ENV:Fault> from
the service, is an indication of service failure or rejection by the
service.
Also, the distinguishing characteristic ID attribute value
"transactionID" of the service can be appended to the
<TransactionResult>. If this "transactionID" attribute is used, if a
COMMIT or ROLLBACK request is received from the transaction service in
the next step, it is possible to make adjustment within the service.

3.2. COMMIT And ROLLBACK Requests
If the Transaction Service receives a successful("SUCCESS") response
from the service, it sends a request message to the next service in
accordance with the order described in the <transactionControl> block.
If SUCCESS is received from all the services, the transaction service
again sends a COMMIT request message with the attached "transactionID"
attribute(Ex. 4) to all the endpoints and makes a decision to execute
the series of processes.
If a service returns a fail message before SUCCESS has been received
from all the services, then the group of services which to this point
had returned SUCCESS are sent a ROLLBACK request message with the
attached "transactionID".

3.3. Processing Completion
The service that receives a COMMIT request processes it and
terminates, and the service that receives a ROLLBACK request deletes
the processing for the service.
Furthermore, the transaction services each reply with their process
completed SOAP message.


Ex.2 (A) Sending a SOAP Envelope to Bank A

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

Ex.2 (B) Sending a SOAP Envelope to Bank B

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

Ex.3 Example Response from Bank A

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


Ex.4 COMMIT Request From Transaction Service to Bank A

<?xml version="1.0" encoding="UTF-8"?>
<SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/">
<SOAP-ENV:Body>
	<t:TransactionAction xmlns:t="http://services.opensoap.jp/transaction/" transactionID="abank13386751012192663">
	COMMIT
	</t:TransactionAction>
</SOAP-ENV:Body>
</SOAP-ENV:Envelope>


Ex.5 Response from Bank A to COMMIT Request

<?xml version="1.0" encoding="UTF-8"?>
<SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/">
<SOAP-ENV:Body>
	<t:TransactionActionResponse xmlns:t="http://services.opensoap.jp/transaction/" transactionID="abank13386751012192663">
	COMMITED
	</t:TransactionActionResponse>
</SOAP-ENV:Body>
</SOAP-ENV:Envelope>

4. Responding To The Client
The transaction service compiles a single Envelope with the same
format as a request message conataining the results of a series of
processes and returns it to the client. The overall transaction
result, defined as a COMMIT or ROLLBACK, is contained in the first
body block, the <TransactionResponse> Block, of the SOAP
message. Subsequent body blocks are made up of the corresponding
blocks returned from each of the services as follows:
  Response Message Header Block -> <TransactionHeaderBlock>
  Body Block -> <TransactionBodyBlock>
for each of the services. Furthermore, on the transmission of a COMMIT
or ROLLBACK, the same attribute value as that in the attached request
message attribute "transactionRequestID" is added to the
<TransactionActionResponseBodyBlock> of the response message body block.


Ex.6 Transaction Process Result Response To Client

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
