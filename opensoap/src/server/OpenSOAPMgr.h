/*
 *-----------------------------------------------------------------------------
 * File		: OpenSOAPMgr.h
 * Version	: 1.0
 * Project	: OPenSOAP
 * Module	: OpenSOAP Process Manager
 * Date		: 2003/12/02
 * Author	: Conchan
 * Copyright(C) Technoface Corporation
 * http://www.technoface.co.jp/
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include	<sys/types.h>

#include	"ServerCommon.h"

#include	"TraceTypes.h"

#include	"TraceMgr.h"
#include	"SharedMgr.h"
#include	"FileMgr.h"

using namespace std;

#include	<cctype>

#define		N_OSPROCS		8
#define		U_OSPROCS		8
#define		MAX_TRIES		5

char		*OSRootDir = "/usr/local/opensoap/sbin/";

char		*OSProcess[N_OSPROCS][3] = {
			"srvConfAttrMgr", "", "srvConfAttrMgr.pid",
			"ssmlAttrMgr", "", "ssmlAttrMgr.pid",
			"idManager", "", "idManager.pid",
			"msgDrvCreator", "", "msgDrvCreator.pid",
			"queueManager", "", "queueManager_req.pid",
			"queueManager", "fwd", "queueManager_fwd.pid",
			"spoolManager", "", "spoolManager.pid",
			"ttlManager", "", "ttlManager.pid"
		};

void		MgrInit(void);
void		MgrSignals(void);
int		MgrMigration(void);
void		MgrRun(int *);
int		MgrForkExec(char **, int *, const char *);
void		MgrSignalHandler(int);
void		MgrProcSetStatus(int, bool);
void		MgrProcRestart(int);
void		MgrCleanZombies(void);
void		MgrHalt(int);
int		MgrProcExist(const char *);
bool		MgrVerifyProcPID(int, const char *);
bool		MgrVerifyKilledProc(int, const char *);



