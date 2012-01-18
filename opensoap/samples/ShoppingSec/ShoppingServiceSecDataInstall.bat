@echo off

set INSTALLDRIVE=C:
set VARDIR=%INSTALLDRIVE%\var
set VARTMPDIR=%VARDIR%\tmp

rem OpenSOAP directories
set OPENSOAPLOCALSTATEDIR=%VARTMPDIR%\OpenSOAP
set OPENSOAPCONFDIR=%OPENSOAPLOCALSTATEDIR%\conf
set OPENSOAPSSMLDIR=%OPENSOAPCONFDIR%\ssml

rem ShoppingServiceSec directories
set SERVICEROOTDIR=%VARTMPDIR%\Shopping
set SERVICECONFDIR=%SERVICEROOTDIR%\conf
set SERVICEDATADIR=%SERVICEROOTDIR%\data
set SERVICEKEYDIR=%SERVICEROOTDIR%\keys

rem ShoppingServiceSec log directories
set SERVICELOGDIR=%OPENSOAPLOCALSTATEDIR%\log

echo create install folders
for %%d in (%VARDIR% %VARTMPDIR% %OPENSOAPLOCALSTATEDIR% %OPENSOAPCONFDIR% %OPENSOAPSSMLDIR% %SERVICEROOTDIR% %SERVICECONFDIR% %SERVICEDATADIR% %SERVICEKEYDIR% %SERVICELOGDIR%) do if not exist %%d mkdir %%d

echo ShoppingSec conf files install
if exist Shopping.conf copy Shopping.conf %SERVICECONFDIR% /Y
rem if exist Shopping.ssml copy Shopping.ssml %OPENSOAPSSMLDIR% /Y

echo ShoppingSec data files install
for %%d in (ProductSpec.data ProductStock.data) do if exist %%d copy %%d %SERVICEDATADIR% /Y

echo ShoppingSec key files install
for %%k in (privKey_ShoppingService.pem pubKey_ShoppingClient.pem Shopping.keys) do if exist %%k copy %%k %SERVICEKEYDIR% /Y
