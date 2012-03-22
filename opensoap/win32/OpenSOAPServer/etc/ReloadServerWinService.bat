@echo off
rem
rem OpenSOAP server configure process reboot
rem

net stop OpenSOAPSrvConfAttrMgr
net stop OpenSOAPSSMLAttrMgr

net start OpenSOAPSrvConfAttrMgr
net start OpenSOAPSSMLAttrMgr

