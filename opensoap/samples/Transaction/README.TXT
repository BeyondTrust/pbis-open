OpenSOAP Transaction Service Sample Document

2003/1/31

The Transaction Management Service is located in /src/tools/Transaction/ .

Documentation relating to the transaction service is located in
/doc/tools/Transaction/ .
What follows is a detailed description of the OpenSOAP Transaction used by
the sample service.

Source code for the sample client and service programs that use the OpenSOAP
Transaction capability can be found in this directory.
The sample service is a deposit/withdrawl service for a virtual bank.

* TransactionClient
  Client program for the Bank Transfer service.
  This command is executed using the command line.

  Command usage:
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
  CGI GUI client program .

* TransactionABankService
  A virtual banking service using the OpenSOAP transaction capability
  that accepts deposit and withdrawl methods.
  Accessing accounts other than those specified in the declarations results
  in an error.

  Request Method

  PaymentRequest: Withdrawl Request. Has account number and amount as
                  attributes.
  DepositRequest: Deposit request. Has account number and amount as
                  attributes.
  TransactionAction: Accepts "commit" or "rollback".
