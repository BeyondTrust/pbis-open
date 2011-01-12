@echo off
rem
rem This file is opensoap/win32/OpenSOAPTools/installOpenSOAPTools.bat
rem For constructing OpenSOAP Server Tools for Windows2000 environment
rem

rem
rem set up your system env. for apache
rem
set APACHE_CGI_DIR="%SystemDrive%\Program Files\Apache Group\Apache\cgi-bin"

rem
rem set up OpenSOAP module install directory
rem
set OPENSOAP_INST_DIR=%SystemDrive%\OpenSOAP
set OPENSOAP_SSML_DIR=%SystemDrive%\usr\local\opensoap\etc\ssml

rem
rem Set Up Services Area
rem
set OPENSOAP_SERVICES=%SystemDrive%\usr\local\opensoap\services
set OPENSOAP_SHOPSEC_KEYS=%OPENSOAP_SERVICES%\ShoppingSec

echo md %OPENSOAP_SERVICES%
md %OPENSOAP_SERVICES%
echo md %OPENSOAP_SHOPSEC_KEYS%
md %OPENSOAP_SHOPSEC_KEYS%


rem
rem Set Up Services Data/Log Area
rem
set OPENSOAP_SERVICES_LOG=%SystemDrive%\usr\local\opensoap\var\services
set OPENSOAP_SHOPSEC_LOG=%OPENSOAP_SERVICES_LOG%\ShoppingSec

echo md %OPENSOAP_SERVICES_LOG%
md %OPENSOAP_SERVICES_LOG%
echo md %OPENSOAP_SHOPSEC_LOG%
md %OPENSOAP_SHOPSEC_LOG%


rem
rem set exe install source and destination
rem
set INST_SOURCE=Release_DLL
set INST_SOURCE_SSML=.
set INST_DEST=%OPENSOAP_INST_DIR%\user\bin
rem INST_DEST_SSML=%OPENSOAP_SSML_DIR%
set INST_SOURCE_KEYS=..\..\samples\ShoppingSec
set INST_SOURCE_KEYS_DESC=.


rem
rem install target exe
rem
rem 	CalcAsyncClient.exe
rem 	CalcAsyncService.exe
rem 	HelloClient.exe
rem 	HelloService.exe
rem 	ShoppingClientSec.exe
rem 	ShoppingServiceSec.exe
rem 	SimpleCalcClient.exe
rem 	SimpleCalcService.exe
rem 	TransactionClient.exe
rem 	TransactionABankService.exe

echo copy %INST_SOURCE%\CalcAsyncClient.exe		%INST_DEST%
copy %INST_SOURCE%\CalcAsyncClient.exe			%INST_DEST%
echo copy %INST_SOURCE%\CalcAsyncService.exe		%INST_DEST%
copy %INST_SOURCE%\CalcAsyncService.exe			%INST_DEST%
echo copy %INST_SOURCE%\HelloClient.exe			%INST_DEST%
copy %INST_SOURCE%\HelloClient.exe			%INST_DEST%
echo copy %INST_SOURCE%\HelloService.exe		%INST_DEST%
copy %INST_SOURCE%\HelloService.exe			%INST_DEST%
echo copy %INST_SOURCE%\ShoppingClientSec.exe		%INST_DEST%
copy %INST_SOURCE%\ShoppingClientSec.exe		%INST_DEST%
echo copy %INST_SOURCE%\ShoppingServiceSec.exe		%INST_DEST%
copy %INST_SOURCE%\ShoppingServiceSec.exe		%INST_DEST%
echo copy %INST_SOURCE%\SimpleCalcClient.exe		%INST_DEST%
copy %INST_SOURCE%\SimpleCalcClient.exe			%INST_DEST%
echo copy %INST_SOURCE%\SimpleCalcService.exe		%INST_DEST%
copy %INST_SOURCE%\SimpleCalcService.exe		%INST_DEST%
echo copy %INST_SOURCE%\TransactionClient.exe		%INST_DEST%
copy %INST_SOURCE%\TransactionClient.exe		%INST_DEST%
echo copy %INST_SOURCE%\TransactionABankService.exe	%INST_DEST%
copy %INST_SOURCE%\TransactionABankService.exe		%INST_DEST%


rem
rem install cgi
rem
rem 	CalcAsyncService.cgi.exe
rem 	HelloService.cgi.exe
rem 	ShoppingServiceSec.cgi.exe
rem 	SimpleCalcService.cgi.exe
rem 	TransactionABankService.cgi.exe

echo copy %INST_SOURCE%\CalcAsyncService.cgi.exe		%APACHE_CGI_DIR%
copy %INST_SOURCE%\CalcAsyncService.cgi.exe			%APACHE_CGI_DIR%
echo copy %INST_SOURCE%\HelloService.cgi.exe			%APACHE_CGI_DIR%
copy %INST_SOURCE%\HelloService.cgi.exe				%APACHE_CGI_DIR%
echo copy %INST_SOURCE%\ShoppingServiceSec.cgi.exe		%APACHE_CGI_DIR%
copy %INST_SOURCE%\ShoppingServiceSec.cgi.exe			%APACHE_CGI_DIR%
echo copy %INST_SOURCE%\SimpleCalcService.cgi.exe		%APACHE_CGI_DIR%
copy %INST_SOURCE%\SimpleCalcService.cgi.exe			%APACHE_CGI_DIR%
echo copy %INST_SOURCE%\TransactionABankService.cgi.exe		%APACHE_CGI_DIR%
copy %INST_SOURCE%\TransactionABankService.cgi.exe		%APACHE_CGI_DIR%


rem
rem install ssml
rem
rem 	CalcAsync.ssml
rem 	Hello.ssml
rem 	ShoppingSec.ssml
rem 	SimpleCalc.ssml
rem	TransactionABank.ssml

echo copy %INST_SOURCE_SSML%\CalcAsync.ssml		%OPENSOAP_SSML_DIR%
copy %INST_SOURCE_SSML%\CalcAsync.ssml			%OPENSOAP_SSML_DIR%
echo copy %INST_SOURCE_SSML%\Hello.ssml			%OPENSOAP_SSML_DIR%
copy %INST_SOURCE_SSML%\Hello.ssml			%OPENSOAP_SSML_DIR%
echo copy %INST_SOURCE_SSML%\ShoppingSec.ssml		%OPENSOAP_SSML_DIR%
copy %INST_SOURCE_SSML%\ShoppingSec.ssml		%OPENSOAP_SSML_DIR%
echo copy %INST_SOURCE_SSML%\SimpleCalc.ssml		%OPENSOAP_SSML_DIR%
copy %INST_SOURCE_SSML%\SimpleCalc.ssml			%OPENSOAP_SSML_DIR%
echo copy %INST_SOURCE_SSML%\TransactionABank.ssml	%OPENSOAP_SSML_DIR%
copy %INST_SOURCE_SSML%\TransactionABank.ssml		%OPENSOAP_SSML_DIR%


rem
rem install certs(Client)
rem
rem	privKey_ShoppingClient.pem			For Client Program
rem	pubKey_ShoppingService.pem			For Client Program
rem

echo copy %INST_SOURCE_KEYS%\privKey_ShoppingClient.pem		%INST_DEST%
copy %INST_SOURCE_KEYS%\privKey_ShoppingClient.pem		%INST_DEST%
echo copy %INST_SOURCE_KEYS%\pubKey_ShoppingService.pem		%INST_DEST%
copy %INST_SOURCE_KEYS%\pubKey_ShoppingService.pem		%INST_DEST%


rem
rem install certs(Service)
rem
rem	pubKey_ShoppingClient.pem			For Service Program
rem	privKey_ShoppingService.pem			For Service Program
rem	Shopping.keys					For Service Program
rem

echo copy %INST_SOURCE_KEYS%\pubKey_ShoppingClient.pem		%OPENSOAP_SHOPSEC_KEYS%
copy %INST_SOURCE_KEYS%\pubKey_ShoppingClient.pem		%OPENSOAP_SHOPSEC_KEYS%
echo copy %INST_SOURCE_KEYS%\privKey_ShoppingService.pem	%OPENSOAP_SHOPSEC_KEYS%
copy %INST_SOURCE_KEYS%\privKey_ShoppingService.pem		%OPENSOAP_SHOPSEC_KEYS%
echo copy %INST_SOURCE_KEYS_DESC%\Shopping.keys			%OPENSOAP_SHOPSEC_KEYS%
copy %INST_SOURCE_KEYS_DESC%\Shopping.keys			%OPENSOAP_SHOPSEC_KEYS%


rem
rem install products data
rem
rem	ProductSpec.data				For ShoppingSec Service
rem	ProductStock.data				For ShoppingSec Service

echo copy %INST_SOURCE_KEYS%\ProductSpec.data		%OPENSOAP_SHOPSEC_LOG%
copy %INST_SOURCE_KEYS%\ProductSpec.data		%OPENSOAP_SHOPSEC_LOG%
echo copy %INST_SOURCE_KEYS%\ProductStock.data		%OPENSOAP_SHOPSEC_LOG%
copy %INST_SOURCE_KEYS%\ProductStock.data		%OPENSOAP_SHOPSEC_LOG%


