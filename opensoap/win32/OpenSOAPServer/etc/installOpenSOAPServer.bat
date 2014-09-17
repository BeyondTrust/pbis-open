@echo off
rem
rem This file is opensoap/win32/etc/installOpenSOAPServer.bat
rem For constructing OpenSOAP Server for Windows2000 environment
rem

rem
rem set up your system env. for apache
rem
set APACHE_CGI_DIR="%SystemDrive%\Program Files\Apache Group\Apache\cgi-bin"

rem
rem set up OpenSOAP module install directory
rem
set OPENSOAP_INST_DIR=%SystemDrive%\OpenSOAP

echo md %OPENSOAP_INST_DIR%\bin
md  %OPENSOAP_INST_DIR%\bin
echo md %OPENSOAP_INST_DIR%\etc
md  %OPENSOAP_INST_DIR%\etc
echo md %OPENSOAP_INST_DIR%\user\bin
md  %OPENSOAP_INST_DIR%\user\bin

rem
rem install dll
rem 

rem install dll and exe

rem
rem set install source and destination
rem
set INST_SOURCE=..\Release
set INST_SOURCE_ETC=.
set INST_SOURCE_CONF=%INST_SOURCE_ETC%
set INST_DEST=%OPENSOAP_INST_DIR%\bin
set INST_DEST_ETC=%OPENSOAP_INST_DIR%\etc

rem 
rem install target dll
rem     - server -
rem     libconnection.dll
rem     libHTTPMessage.dll
rem     libOpenSOAPServer.dll
rem     - api -
rem     OpenSOAPClient.dll
rem     OpenSOAPSecurity.dll
rem     OpenSOAPService.dll
set INST_SOURCE_API=..\..\OpenSOAP\Release_DLL

echo copy %INST_SOURCE%\libconnection.dll       %INST_DEST%
copy %INST_SOURCE%\libconnection.dll            %INST_DEST%
echo copy %INST_SOURCE%\libHTTPMessage.dll      %INST_DEST%
copy %INST_SOURCE%\libHTTPMessage.dll           %INST_DEST%
echo copy %INST_SOURCE%\libOpenSOAPServer.dll       %INST_DEST%
copy %INST_SOURCE%\libOpenSOAPServer.dll        %INST_DEST%

echo copy %INST_SOURCE_API%\OpenSOAPClient.dll      %INST_DEST%
copy %INST_SOURCE_API%\OpenSOAPClient.dll       %INST_DEST%
echo copy %INST_SOURCE_API%\OpenSOAPSecurity.dll    %INST_DEST%
copy %INST_SOURCE_API%\OpenSOAPSecurity.dll     %INST_DEST%
echo copy %INST_SOURCE_API%\OpenSOAPService.dll     %INST_DEST%
copy %INST_SOURCE_API%\OpenSOAPService.dll      %INST_DEST%

rem
rem install target exe
rem
rem     idManager
rem     srvConfAttrMgr
rem     ssmlAttrMgr
rem     msgDrvCreator
rem     queueManager
rem     spoolManager
rem     ttlManager

echo copy %INST_SOURCE%\idManager.exe           %INST_DEST%
copy %INST_SOURCE%\idManager.exe            %INST_DEST%
echo copy %INST_SOURCE%\srvConfAttrMgr.exe      %INST_DEST%
copy %INST_SOURCE%\srvConfAttrMgr.exe           %INST_DEST%
echo copy %INST_SOURCE%\ssmlAttrMgr.exe         %INST_DEST%
copy %INST_SOURCE%\ssmlAttrMgr.exe          %INST_DEST%
echo copy %INST_SOURCE%\msgDrvCreator.exe       %INST_DEST%
copy %INST_SOURCE%\msgDrvCreator.exe            %INST_DEST%
echo copy %INST_SOURCE%\queueManager.exe        %INST_DEST%
copy %INST_SOURCE%\queueManager.exe         %INST_DEST%
echo copy %INST_SOURCE%\spoolManager.exe        %INST_DEST%
copy %INST_SOURCE%\spoolManager.exe         %INST_DEST%
echo copy %INST_SOURCE%\ttlManager.exe          %INST_DEST%
copy %INST_SOURCE%\ttlManager.exe           %INST_DEST%

rem  for windows service setup program
echo copy %INST_SOURCE%\idManagerWinService.exe         %INST_DEST%
copy %INST_SOURCE%\idManagerWinService.exe          %INST_DEST%
echo copy %INST_SOURCE%\srvConfAttrMgrWinService.exe        %INST_DEST%
copy %INST_SOURCE%\srvConfAttrMgrWinService.exe         %INST_DEST%
echo copy %INST_SOURCE%\ssmlAttrMgrWinService.exe       %INST_DEST%
copy %INST_SOURCE%\ssmlAttrMgrWinService.exe            %INST_DEST%
echo copy %INST_SOURCE%\msgDrvCreatorWinService.exe     %INST_DEST%
copy %INST_SOURCE%\msgDrvCreatorWinService.exe          %INST_DEST%
echo copy %INST_SOURCE%\queueManagerWinService.exe      %INST_DEST%
copy %INST_SOURCE%\queueManagerWinService.exe           %INST_DEST%
echo copy %INST_SOURCE%\queueManagerFwdWinService.exe       %INST_DEST%
copy %INST_SOURCE%\queueManagerFwdWinService.exe        %INST_DEST%
echo copy %INST_SOURCE%\spoolManagerWinService.exe      %INST_DEST%
copy %INST_SOURCE%\spoolManagerWinService.exe           %INST_DEST%
echo copy %INST_SOURCE%\ttlManagerWinService.exe        %INST_DEST%
copy %INST_SOURCE%\ttlManagerWinService.exe         %INST_DEST%

rem
rem install cgi
rem
echo copy %INST_SOURCE%\soapInterface.cgi       %APACHE_CGI_DIR%
copy %INST_SOURCE%\soapInterface.cgi            %APACHE_CGI_DIR%

rem
rem install utility
rem 
echo copy %INST_SOURCE_ETC%\ServerWinService.bat            %INST_DEST_ETC%
copy %INST_SOURCE_ETC%\ServerWinService.bat     %INST_DEST_ETC%
echo copy %INST_SOURCE_ETC%\ServerWinServiceNetCmd.bat  %INST_DEST_ETC%
copy %INST_SOURCE_ETC%\ServerWinServiceNetCmd.bat   %INST_DEST_ETC%
echo copy %INST_SOURCE_ETC%\DeleteServerWinService.bat  %INST_DEST_ETC%
copy %INST_SOURCE_ETC%\DeleteServerWinService.bat   %INST_DEST_ETC%
echo copy %INST_SOURCE_ETC%\ReloadServerWinService.bat  %INST_DEST_ETC%
copy %INST_SOURCE_ETC%\ReloadServerWinService.bat   %INST_DEST_ETC%
echo copy %INST_SOURCE_ETC%\SetupServerWinService.bat   %INST_DEST_ETC%
copy %INST_SOURCE_ETC%\SetupServerWinService.bat    %INST_DEST_ETC%
echo copy %INST_SOURCE_ETC%\StartServerWinService.bat   %INST_DEST_ETC%
copy %INST_SOURCE_ETC%\StartServerWinService.bat    %INST_DEST_ETC%
echo copy %INST_SOURCE_ETC%\StopServerWinService.bat    %INST_DEST_ETC%
copy %INST_SOURCE_ETC%\StopServerWinService.bat     %INST_DEST_ETC%

rem
rem install server configuration file
rem 
echo copy %INST_SOURCE_CONF%\server.conf        %INST_DEST_ETC%
copy %INST_SOURCE_CONF%\server.conf         %INST_DEST_ETC%

rem
rem set up OpenSOAP run environment Root directory
rem

rem --------------------------------------------------------------------------
rem When you change OPENSOAP_ROOT, please change %INST_DEST_ETC%\server.conf 
rem and %INST_DEST_ETC%\StartServerWinService.bat similarly.
rem --------------------------------------------------------------------------
rem set OPENSOAP_ROOT=%SystemDrive%\opensoap
set OPENSOAP_ROOT=%SystemDrive%\usr\local\opensoap

rem
rem create directory for run env.
rem 
echo md %OPENSOAP_ROOT%\var\log
md  %OPENSOAP_ROOT%\var\log
echo md %OPENSOAP_ROOT%\var\spool
md  %OPENSOAP_ROOT%\var\spool
echo md %OPENSOAP_ROOT%\etc\ssml
md  %OPENSOAP_ROOT%\etc\ssml


