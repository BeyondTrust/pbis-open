@echo off
rem
rem Windows Service start/stop cntl
rem

net %1 OpenSOAPSrvConfAttrMgr
net %1 OpenSOAPSSMLAttrMgr
net %1 OpenSOAPIdManager
net %1 OpenSOAPMsgDrvCreator
net %1 OpenSOAPQueueManager
net %1 OpenSOAPQueueManagerFwd
net %1 OpenSOAPSpoolManager
net %1 OpenSOAPTTLManager

