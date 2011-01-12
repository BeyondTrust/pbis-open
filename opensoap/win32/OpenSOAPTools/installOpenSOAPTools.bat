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
rem set exe install source and destination
rem
set INST_SOURCE=Release_DLL
set INST_SOURCE_SSML=.
set INST_DEST=%OPENSOAP_INST_DIR%\user\bin
rem INST_DEST_SSML=%OPENSOAP_SSML_DIR%


rem
rem install target exe
rem
rem 	Soaping.exe
rem 	SoapingService.exe
rem 	TransactionService.exe

echo mkdir %INST_DEST%
mkdir %INST_DEST%
echo copy %INST_SOURCE%\Soaping.exe			%INST_DEST%
copy %INST_SOURCE%\Soaping.exe				%INST_DEST%
echo copy %INST_SOURCE%\SoapingService.exe		%INST_DEST%
copy %INST_SOURCE%\SoapingService.exe			%INST_DEST%
echo copy %INST_SOURCE%\TransactionService.exe		%INST_DEST%
copy %INST_SOURCE%\TransactionService.exe		%INST_DEST%


rem
rem install cgi
rem
rem 	SoapingService.cgi.exe
rem 	TransactionService.cgi.exe

echo mkdir %APACHE_CGI_DIR%
mkdir %APACHE_CGI_DIR%
echo copy %INST_SOURCE%\SoapingService.cgi.exe		%APACHE_CGI_DIR%
copy %INST_SOURCE%\SoapingService.cgi.exe		%APACHE_CGI_DIR%
echo copy %INST_SOURCE%\TransactionService.cgi.exe	%APACHE_CGI_DIR%
copy %INST_SOURCE%\TransactionService.cgi.exe		%APACHE_CGI_DIR%


rem
rem install ssml
rem
rem 	Soaping.ssml
rem	Transaction.ssml

mkdir %OPENSOAP_SSML_DIR%
echo mkdir %OPENSOAP_SSML_DIR%
echo copy %INST_SOURCE_SSML%\Soaping.ssml		%OPENSOAP_SSML_DIR%
copy %INST_SOURCE_SSML%\Soaping.ssml			%OPENSOAP_SSML_DIR%
echo copy %INST_SOURCE_SSML%\Transaction.ssml		%OPENSOAP_SSML_DIR%
copy %INST_SOURCE_SSML%\Transaction.ssml		%OPENSOAP_SSML_DIR%
