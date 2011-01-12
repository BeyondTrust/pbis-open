@echo off
rem
rem Windows Service setup 
rem
rem %1 = [setup|delete]
rem

idManagerWinService 			%1
srvConfAttrMgrWinService 		%1
ssmlAttrMgrWinService 			%1
msgDrvCreatorWinService 		%1
queueManagerWinService 			%1
queueManagerFwdWinService 		%1
spoolManagerWinService 			%1
ttlManagerWinService 			%1

