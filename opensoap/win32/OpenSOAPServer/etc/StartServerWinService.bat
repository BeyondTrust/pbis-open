@echo off
rem
rem OpenSOAP Server process start up
rem

rem
rem OpenSOAP Root directry
rem 
rem set OPENSOAP_ROOT=%SystemDrive%\opensoap
set OPENSOAP_ROOT=%SystemDrive%\usr\local\opensoap

rem
rem clean up 
rem
del /Q /F %OPENSOAP_ROOT%\var\spool\*

rem
rem start OpenSOAP Server process
rem
ServerWinServiceNetCmd start

