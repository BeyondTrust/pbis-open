Shopping Sample
===============


[1] Outline :

  This directory contains the sample source code for a simple B2C-EC shopping
  web service implemented using the SOAP protocol.

  The Shopping web service is made up of the following 4 methods.

  Method           Meaning                    Parameter1    Parameter2    Return Value
  ---------------  ------------               ------------  ------------  --------------
  GetProductCount  Get no. of product types   none          none          No. of product types
  GetProductSpec   Get product details        index         none          Product details
  GetStockQty      Get remaining stock        Product code  none          Remaining stock
  PlaceOrder       Place order                Product code  number        Order success/failure

  For examples of request/response messages for each method, please refer
  to the corresponding files in the SampleMessages directory.

  For the client program, the product list is acquired using the GetProductList
  method which is a combination of GetProductCount and GetProductSpec.


[2] Building the executables :

  After installing the OpenSOAP API, use the following make files to build
  the sample programs.

  (1) Shopping.mak  --- Synchronous & Asynchronous versions
                (Only the PlaceOrder service can be used in either synchronous
                 or asynchronous mode)

    * ShoppingService
      Standard I/O version of the sample shopping service program.

    * ShoppingService.cgi
      CGI version of the sample Shopping service program.

    * ShoppingClient
      Synchronous shopping service sample client program.

    * ShoppingClientAsync
      Asynchronous shopping service sample client program.
      (Only PlaceOrder method is asynchronous)
      Asynchronous processing can be specified when the service is called
      through the OpenSOAP server.

  (2) ShoppingSec.mak   --- Security version
                (Only PlaceOrder is encrypted and has an attached signature)

    * ShoppingServiceSec
      Secure standard I/O version of the sample shopping service program.

    * ShoppingServiceSec.cgi
      Secure CGI version of the sample shopping service program.

    * ShoppingClientSec
      Secure shopping service sample client program.

    * ShoppingClientSec.cgi
      Secure CGI shopping service sample client program.


[3] Installation :

  The program files, configuration files and the data file must be installed in
  the proper locations.

  In general, using
    make -f "make file name" install
  will install the files in their proper directories.

  For more details, please refer to the corresponding make files.


[4] Execution : 

  Please refer to the make files for instructions on running the programs.
  Rules for sample execution are described at the bottom of the make file.

  make -f Shopping.mak *test*

      Test of service using sample request message
      --------------------------------------------
    (Rule)    (Description)                  (Called Service Method Name)
     stest1    Get count of product types     GetProductCount
     stest2    Get product description        GetProductSpec
     stest3    Get product available stock    GetStockQty
     stest4    Submit an order                PlaceOrder

     stest5    Get count of product types     GetProductCount  (Fail example)
     stest6    Get product description        GetProductSpec   (Fail example)
     stest7    Get product available stock    GetStockQty      (Fail example)
     stest8    Submit an order                PlaceOrder       (Fail example)


      Test of client
      --------------
     ctest1    Description of all products    GetProductCount + GetProductSpec
     ctest2    Get product available stock    GetStockQty
     stest3    Submit an order                PlaceOrder
     ctest1a   (ctest1 test using Asynchronous client ShoppingClientAsync)
     ctest2a   (ctest2 test using Asynchronous client ShoppingClientAsync)
     stest3a   (ctest3 test using Asynchronous client ShoppingClientAsync)


      Test of client(via OpenSOAP Server)
      -----------------------------------
    * ShoppingClient(Synchronous)
     cstest1   Description of all products    GetProductCount + GetProductSpec
     cstest2   Get product available stock    GetStockQty
     cstest3   Submit an order                PlaceOrder

    * ShoppingClientAsync(Asynchronous)
     cstest1a  Description of all products    GetProductCount + GetProductSpec
               (This, as for cstest1 is a synchronous operation)
     cstest2a  Get product available stock    GetStockQty
               (This, as for cstest2 is a synchronous operation)
     cstest3a  Submit an order                PlaceOrder
               (Synchronous processing of order submission)
     cstest4a  Submit an order                PlaceOrderAsync
               (Asynchronous processing of order submission)
     cstest5a  Get result of cstest4a         ConfirmOrder


  make -f ShoppingSec.mak *test*

      Test of service using sample request message
      --------------------------------------------
     stest1    Get count of product types     GetProductCount
     stest2    Get product description        GetProductSpec
     stest3    Get product available stock    GetStockQty
     stest4    Submit an order                PlaceOrder


      Test of client
      --------------
     ctest1    Description of all products    GetProductCount + GetProductSpec
     ctest2    Get product available stock    GetStockQty
     ctest3    Submit an order                PlaceOrder


      Test of client(via OpenSOAP Server)
      -----------------------------------
     cstest1   Description of all products    GetProductCount + GetProductSpec
     cstest2   Get product available stock    GetStockQty
     cstest3   Submit an order                PlaceOrder

  The rule "ctest1", for example, can be run as follows:

  For the security version only, the web application client ShoppingClientSec.cgi
  is included and can be run usin a web browser.

  Use a URL similar to the following in the browser to run this application.

  http://localhost/cgi-bin/ShoppingClientSec.cgi

