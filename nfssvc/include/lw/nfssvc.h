/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        nfssvc.h
 *
 * Abstract:
 *
 *        Likewise Server Service (nfssvc) RPC client and server
 *
 *        Client API
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _NFSSVC_H_
#define _NFSSVC_H_

#include <lwrpc/types.h>
#include <lwio/lmshare.h>


#ifndef CONNECTION_INFO_0_DEFINED
#define CONNECTION_INFO_0_DEFINED 1

typedef struct _CONNECTION_INFO_0 {
    UINT32 coni0_id;
} CONNECTION_INFO_0, *PCONNECTION_INFO_0;

#endif

typedef struct {
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    CONNECTION_INFO_0 *array;
} nfssvc_NetConnCtr0;


#ifndef CONNECTION_INFO_1_DEFINED
#define CONNECTION_INFO_1_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef CONNECTION_INFO_1_DEFINED")
cpp_quote("#define CONNECTION_INFO_1_DEFINED 1")
#endif

typedef struct _CONNECTION_INFO_1 {
    UINT32 coni1_id;
    UINT32 coni1_type;
    UINT32 coni1_num_open;
    UINT32 coni1_num_users;
    UINT32 coni1_time;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR coni1_username;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR coni1_netname;
} CONNECTION_INFO_1, *PCONNECTION_INFO_1;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

typedef struct {
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    CONNECTION_INFO_1 *array;
} nfssvc_NetConnCtr1;


#ifndef _DCE_IDL_

typedef union {
    nfssvc_NetConnCtr0 *ctr0;
    nfssvc_NetConnCtr1 *ctr1;
} nfssvc_NetConnCtr;

#endif


#ifndef FILE_INFO_2_DEFINED
#define FILE_INFO_2_DEFINED 1

typedef struct _FILE_INFO_2 {
    UINT32 fi2_id;
} FILE_INFO_2, *PFILE_INFO_2;

#endif

typedef struct {
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    FILE_INFO_2 *array;
} nfssvc_NetFileCtr2;


#ifndef FILE_INFO_3_DEFINED
#define FILE_INFO_3_DEFINED 1

typedef struct _FILE_INFO_3 {
    UINT32 fi3_idd;
    UINT32 fi3_permissions;
    UINT32 fi3_num_locks;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *fi3_path_name;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *fi3_username;
} FILE_INFO_3, *PFILE_INFO_3;

#endif

typedef struct {
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    FILE_INFO_3 *array;
} nfssvc_NetFileCtr3;


#ifndef _DCE_IDL_

typedef union {
    nfssvc_NetFileCtr2 *ctr2;
    nfssvc_NetFileCtr3 *ctr3;
} nfssvc_NetFileCtr;


typedef union {
    FILE_INFO_2 *info2;
    FILE_INFO_3 *info3;
} nfssvc_NetFileInfo;

#endif


#ifndef SESSION_INFO_0_DEFINED
#define SESSION_INFO_0_DEFINED 1

typedef struct _SESSION_INFO_0 {
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi0_cname;
} SESSION_INFO_0, *PSESSION_INFO_0;

#endif

typedef struct {
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    SESSION_INFO_0 *array;
} nfssvc_NetSessCtr0;


#ifndef SESSION_INFO_1_DEFINED
#define SESSION_INFO_1_DEFINED 1

typedef struct _SESSION_INFO_1 {
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi1_cname;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi1_username;
    UINT32 sesi1_num_opens;
    UINT32 sesi1_time;
    UINT32 sesi1_idle_time;
    UINT32 sesi1_user_flags;
} SESSION_INFO_1, *PSESSION_INFO_1;

#endif

typedef struct {
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    SESSION_INFO_1 *array;
} nfssvc_NetSessCtr1;


#ifndef SESSION_INFO_2_DEFINED
#define SESSION_INFO_2_DEFINED 1


typedef struct _SESSION_INFO_2 {
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi2_cname;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi2_username;
    UINT32 sesi2_num_opens;
    UINT32 sesi2_time;
    UINT32 sesi2_idle_time;
    UINT32 sesi2_user_flags;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi2_cltype_name;
} SESSION_INFO_2, *PSESSION_INFO_2;

#endif

typedef struct {
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    SESSION_INFO_2 *array;
} nfssvc_NetSessCtr2;


#ifndef SESSION_INFO_10_DEFINED
#define SESSION_INFO_10_DEFINED 1

typedef struct _SESSION_INFO_10 {
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi10_cname;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi10_username;
    UINT32 sesi10_time;
    UINT32 sesi10_idle_time;
} SESSION_INFO_10, *PSESSION_INFO_10;

#endif

typedef struct {
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    SESSION_INFO_10 *array;
} nfssvc_NetSessCtr10;


#ifndef SESSION_INFO_502_DEFINED
#define SESSION_INFO_502_DEFINED 1

typedef struct _SESSION_INFO_502 {
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi502_cname;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi502_username;
    UINT32 sesi502_num_opens;
    UINT32 sesi502_time;
    UINT32 sesi502_idle_time;
    UINT32 sesi502_user_flags;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi502_cltype_name;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi502_transport;
} SESSION_INFO_502, *PSESSION_INFO_502;

#endif

typedef struct {
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    SESSION_INFO_502 *array;
} nfssvc_NetSessCtr502;


#ifndef _DCE_IDL_

typedef union {
    nfssvc_NetSessCtr0 *ctr0;
    nfssvc_NetSessCtr1 *ctr1;
    nfssvc_NetSessCtr2 *ctr2;
    nfssvc_NetSessCtr10 *ctr10;
    nfssvc_NetSessCtr502 *ctr502;
} nfssvc_NetSessCtr;

#endif


#ifndef SERVER_INFO_100_DEFINED
#define SERVER_INFO_100_DEFINED 1


typedef struct _SERVER_INFO_100 {
    UINT32 sv100_platform_id;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv100_name;
} SERVER_INFO_100, *PSERVER_INFO_100;

#endif


#ifndef SERVER_INFO_101_DEFINED
#define SERVER_INFO_101_DEFINED 1

typedef struct _SERVER_INFO_101 {
    UINT32 sv101_platform_id;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv101_name;
    UINT32 sv101_version_major;
    UINT32 sv101_version_minor;
    UINT32 sv101_type;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv101_comment;
} SERVER_INFO_101, *PSERVER_INFO_101;

#endif


#ifndef SERVER_INFO_102_DEFINED
#define SERVER_INFO_102_DEFINED 1


typedef struct _SERVER_INFO_102 {
    UINT32 sv102_platform_id;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv102_name;
    UINT32 sv102_version_major;
    UINT32 sv102_version_minor;
    UINT32 sv102_type;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv102_comment;
    UINT32 sv102_users;
    UINT32 sv102_disc;
    UINT32 sv102_hidden;
    UINT32 sv102_announce;
    UINT32 sv102_anndelta;
    UINT32 sv102_licenses;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv102_userpath;
} SERVER_INFO_102, *PSERVER_INFO_102;

#endif


#ifndef SERVER_INFO_402_DEFINED
#define SERVER_INFO_402_DEFINED 1


typedef struct _SERVER_INFO_402 {
    UINT32 sv402_ulist_mtime;
    UINT32 sv402_glist_mtime;
    UINT32 sv402_alist_mtime;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv402_alerts;
    UINT32 sv402_security;
    UINT32 sv402_numadmin;
    UINT32 sv402_lanmask;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv402_guestacct;
    UINT32 sv402_chdevs;
    UINT32 sv402_chdevq;
    UINT32 sv402_chdevjobs;
    UINT32 sv402_connections;
    UINT32 sv402_shares;
    UINT32 sv402_openfiles;
    UINT32 sv402_sessopens;
    UINT32 sv402_sesssvcs;
    UINT32 sv402_sessreqs;
    UINT32 sv402_opensearch;
    UINT32 sv402_activelocks;
    UINT32 sv402_numreqbuf;
    UINT32 sv402_sizreqbuf;
    UINT32 sv402_numbigbuf;
    UINT32 sv402_numfiletasks;
    UINT32 sv402_alertsched;
    UINT32 sv402_erroralert;
    UINT32 sv402_logonalert;
    UINT32 sv402_accessalert;
    UINT32 sv402_diskalert;
    UINT32 sv402_netioalert;
    UINT32 sv402_maxaudits;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv402_srvheuristics;
} SERVER_INFO_402, *PSERVER_INFO_402;

#endif


#ifndef SERVER_INFO_403_DEFINED
#define SERVER_INFO_403_DEFINED 1

typedef struct _SERVER_INFO_403 {
    UINT32 sv403_ulist_mtime;
    UINT32 sv403_glist_mtime;
    UINT32 sv403_alist_mtime;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv403_alerts;
    UINT32 sv403_security;
    UINT32 sv403_numadmin;
    UINT32 sv403_lanmask;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv403_guestacct;
    UINT32 sv403_chdevs;
    UINT32 sv403_chdevq;
    UINT32 sv403_chdevjobs;
    UINT32 sv403_connections;
    UINT32 sv403_shares;
    UINT32 sv403_openfiles;
    UINT32 sv403_sessopens;
    UINT32 sv403_sesssvcs;
    UINT32 sv403_sessreqs;
    UINT32 sv403_opensearch;
    UINT32 sv403_activelocks;
    UINT32 sv403_numreqbuf;
    UINT32 sv403_sizereqbuf;
    UINT32 sv403_numbigbuf;
    UINT32 sv403_numfiletasks;
    UINT32 sv403_alertsched;
    UINT32 sv403_erroralert;
    UINT32 sv403_logonalert;
    UINT32 sv403_accessalert;
    UINT32 sv403_diskalert;
    UINT32 sv403_netioalert;
    UINT32 sv403_maxaudits;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv403_srvheuristics;
    UINT32 sv403_auditedevents;
    UINT32 sv403_auditprofile;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv403_autopath;
} SERVER_INFO_403, *PSERVER_INFO_403;

#endif


#ifndef SERVER_INFO_502_DEFINED
#define SERVER_INFO_502_DEFINED 1

typedef struct _SERVER_INFO_502 {
    UINT32 sv502_sessopens;
    UINT32 sv502_sessvcs;
    UINT32 sv502_opensearch;
    UINT32 sv502_sizreqbuf;
    UINT32 sv502_initworkitems;
    UINT32 sv502_maxworkitems;
    UINT32 sv502_rawworkitems;
    UINT32 sv502_irpstacksize;
    UINT32 sv502_maxrawbuflen;
    UINT32 sv502_sessusers;
    UINT32 sv502_sessconns;
    UINT32 sv502_maxpagedmemoryusage;
    UINT32 sv502_maxnonpagedmemoryusage;
    UINT32 sv502_enablesoftcompat;
    UINT32 sv502_enableforcedlogoff;
    UINT32 sv502_timesource;
    UINT32 sv502_acceptdownlevelapis;
    UINT32 sv502_lmannounce;
} SERVER_INFO_502, *PSERVER_INFO_502;

#endif


#ifndef SERVER_INFO_503_DEFINED
#define SERVER_INFO_503_DEFINED 1

typedef struct _SERVER_INFO_503 {
    UINT32 sv503_sessopens;
    UINT32 sv503_sessvcs;
    UINT32 sv503_opensearch;
    UINT32 sv503_sizreqbuf;
    UINT32 sv503_initworkitems;
    UINT32 sv503_maxworkitems;
    UINT32 sv503_rawworkitems;
    UINT32 sv503_irpstacksize;
    UINT32 sv503_maxrawbuflen;
    UINT32 sv503_sessusers;
    UINT32 sv503_sessconns;
    UINT32 sv503_maxpagedmemoryusage;
    UINT32 sv503_maxnonpagedmemoryusage;
    UINT32 sv503_enablesoftcompat;
    UINT32 sv503_enableforcedlogoff;
    UINT32 sv503_timesource;
    UINT32 sv503_acceptdownlevelapis;
    UINT32 sv503_lmannounce;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv503_domain;
    UINT32 sv503_maxcopyreadlen;
    UINT32 sv503_maxcopywritelen;
    UINT32 sv503_minkeepsearch;
    UINT32 sv503_maxkeepsearch;
    UINT32 sv503_minkeepcomplsearch;
    UINT32 sv503_maxkeepcomplsearch;
    UINT32 sv503_threadcountadd;
    UINT32 sv503_numblockthreads;
    UINT32 sv503_scavtimeout;
    UINT32 sv503_minrcvqueue;
    UINT32 sv503_minfreeworkitems;
    UINT32 sv503_xactmemsize;
    UINT32 sv503_threadpriority;
    UINT32 sv503_maxmpxct;
    UINT32 sv503_oplockbreakwait;
    UINT32 sv503_oplockbreakresponsewait;
    UINT32 sv503_enableoplocks;
    UINT32 sv503_enableoplockforceclose;
    UINT32 sv503_enablefcbopens;
    UINT32 sv503_enableraw;
    UINT32 sv503_enablesharednetdrives;
    UINT32 sv503_minfreeconnections;
    UINT32 sv503_maxfreeconnections;
} SERVER_INFO_503, *PSERVER_INFO_503;

#endif


#ifndef SERVER_INFO_599_DEFINED
#define SERVER_INFO_599_DEFINED 1

typedef struct _SERVER_INFO_599 {
    UINT32 sv599_sessopens;
    UINT32 sv599_sessvcs;
    UINT32 sv599_opensearch;
    UINT32 sv599_sizreqbuf;
    UINT32 sv599_initworkitems;
    UINT32 sv599_maxworkitems;
    UINT32 sv599_rawworkitems;
    UINT32 sv599_irpstacksize;
    UINT32 sv599_maxrawbuflen;
    UINT32 sv599_sessusers;
    UINT32 sv599_sessconns;
    UINT32 sv599_maxpagedmemoryusage;
    UINT32 sv599_maxnonpagedmemoryusage;
    UINT32 sv599_enablesoftcompat;
    UINT32 sv599_enableforcedlogoff;
    UINT32 sv599_timesource;
    UINT32 sv599_acceptdownlevelapis;
    UINT32 sv599_lmannounce;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv599_domain;
    UINT32 sv599_maxcopyreadlen;
    UINT32 sv599_maxcopywritelen;
    UINT32 sv599_minkeepsearch;
    UINT32 sv599_maxkeepsearch;
    UINT32 sv599_minkeepcomplsearch;
    UINT32 sv599_maxkeepcomplsearch;
    UINT32 sv599_threadcountadd;
    UINT32 sv599_numblockthreads;
    UINT32 sv599_scavtimeout;
    UINT32 sv599_minrcvqueue;
    UINT32 sv599_minfreeworkitems;
    UINT32 sv599_xactmemsize;
    UINT32 sv599_threadpriority;
    UINT32 sv599_maxmpxct;
    UINT32 sv599_oplockbreakwait;
    UINT32 sv599_oplockbreakresponsewait;
    UINT32 sv599_enableoplocks;
    UINT32 sv599_enableoplockforceclose;
    UINT32 sv599_enablefcbopens;
    UINT32 sv599_enableraw;
    UINT32 sv599_enablesharednetdrives;
    UINT32 sv599_minfreeconnections;
    UINT32 sv599_maxfreeconnections;
    UINT32 sv599_initsesstable;
    UINT32 sv599_initconntable;
    UINT32 sv599_initfiletable;
    UINT32 sv599_initsearchtable;
    UINT32 sv599_alertschedule;
    UINT32 sv599_errorthreshold;
    UINT32 sv599_networkerrorthreshold;
    UINT32 sv599_diskspacethreshold;
    UINT32 sv599_reserved;
    UINT32 sv599_maxlinkdelay;
    UINT32 sv599_minlinkthroughput;
    UINT32 sv599_linkinfovalidtime;
    UINT32 sv599_scavqosinfoupdatetime;
    UINT32 sv599_maxworkitemidletime;
} SERVER_INFO_599, *PSERVER_INFO_599;

#endif


#ifndef SERVER_INFO_1005_DEFINED
#define SERVER_INFO_1005_DEFINED 1

typedef struct _SERVER_INFO_1005 {
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv1005_comment;
} SERVER_INFO_1005, *PSERVER_INFO_1005;

#endif


#ifndef SERVER_INFO_1010_DEFINED
#define SERVER_INFO_1010_DEFINED 1

typedef struct _SERVER_INFO_1010 {
    UINT32 sv1010_disc;
} SERVER_INFO_1010, *PSERVER_INFO_1010;

#endif


#ifndef SERVER_INFO_1016_DEFINED
#define SERVER_INFO_1016_DEFINED 1

typedef struct _SERVER_INFO_1016 {
    UINT32 sv1016_hidden;
} SERVER_INFO_1016, *PSERVER_INFO_1016;

#endif


#ifndef SERVER_INFO_1017_DEFINED
#define SERVER_INFO_1017_DEFINED 1

typedef struct _SERVER_INFO_1017 {
    UINT32 sv1017_announce;
} SERVER_INFO_1017, *PSERVER_INFO_1017;

#endif


#ifndef SERVER_INFO_1018_DEFINED
#define SERVER_INFO_1018_DEFINED 1

typedef struct _SERVER_INFO_1018 {
    UINT32 sv1018_anndelta;
} SERVER_INFO_1018, *PSERVER_INFO_1018;

#endif


#ifndef SERVER_INFO_1107_DEFINED
#define SERVER_INFO_1107_DEFINED 1

typedef struct _SERVER_INFO_1107 {
    UINT32 sv1107_users;
} SERVER_INFO_1107, *PSERVER_INFO_1107;

#endif


#ifndef SERVER_INFO_1501_DEFINED
#define SERVER_INFO_1501_DEFINED 1

typedef struct _SERVER_INFO_1501 {
    UINT32 sv1501_sessopens;
} SERVER_INFO_1501, *PSERVER_INFO_1501;

#endif


#ifndef SERVER_INFO_1502_DEFINED
#define SERVER_INFO_1502_DEFINED 1

typedef struct _SERVER_INFO_1502 {
    UINT32 sv1502_sessvcs;
} SERVER_INFO_1502, *PSERVER_INFO_1502;

#endif


#ifndef SERVER_INFO_1503_DEFINED
#define SERVER_INFO_1503_DEFINED 1

typedef struct _SERVER_INFO_1503 {
    UINT32 sv1503_opensearch;
} SERVER_INFO_1503, *PSERVER_INFO_1503;

#endif


#ifndef SERVER_INFO_1506_DEFINED
#define SERVER_INFO_1506_DEFINED 1

typedef struct _SERVER_INFO_1506 {
    UINT32 sv1506_maxworkitems;
} SERVER_INFO_1506, *PSERVER_INFO_1506;

#endif


#ifndef SERVER_INFO_1509_DEFINED
#define SERVER_INFO_1509_DEFINED 1

typedef struct _SERVER_INFO_1509 {
    UINT32 sv1509_maxrawbuflen;
} SERVER_INFO_1509, *PSERVER_INFO_1509;

#endif


#ifndef SERVER_INFO_1510_DEFINED
#define SERVER_INFO_1510_DEFINED 1

typedef struct _SERVER_INFO_1510 {
    UINT32 sv1510_sessusers;
} SERVER_INFO_1510, *PSERVER_INFO_1510;

#endif


#ifndef SERVER_INFO_1511_DEFINED
#define SERVER_INFO_1511_DEFINED 1

typedef struct _SERVER_INFO_1511 {
    UINT32 sv1511_sessconns;
} SERVER_INFO_1511, *PSERVER_INFO_1511;

#endif


#ifndef SERVER_INFO_1512_DEFINED
#define SERVER_INFO_1512_DEFINED 1

typedef struct _SERVER_INFO_1512 {
    UINT32 sv1512_maxnonpagedmemoryusage;
} SERVER_INFO_1512, *PSERVER_INFO_1512;

#endif


#ifndef SERVER_INFO_1513_DEFINED
#define SERVER_INFO_1513_DEFINED 1

typedef struct _SERVER_INFO_1513 {
    UINT32 sv1513_maxpagedmemoryusage;
} SERVER_INFO_1513, *PSERVER_INFO_1513;

#endif


#ifndef SERVER_INFO_1514_DEFINED
#define SERVER_INFO_1514_DEFINED 1

typedef struct _SERVER_INFO_1514 {
    UINT32 sv1514_enablesoftcompat;
} SERVER_INFO_1514, *PSERVER_INFO_1514;

#endif


#ifndef SERVER_INFO_1515_DEFINED
#define SERVER_INFO_1515_DEFINED 1

typedef struct _SERVER_INFO_1515 {
    UINT32 sv1515_enableforcedlogoff;
} SERVER_INFO_1515, *PSERVER_INFO_1515;

#endif


#ifndef SERVER_INFO_1516_DEFINED
#define SERVER_INFO_1516_DEFINED 1

typedef struct _SERVER_INFO_1516 {
    UINT32 sv1516_timesource;
} SERVER_INFO_1516, *PSERVER_INFO_1516;

#endif


#ifndef SERVER_INFO_1518_DEFINED
#define SERVER_INFO_1518_DEFINED 1

typedef struct _SERVER_INFO_1518 {
    UINT32 sv1518_lmannounce;
} SERVER_INFO_1518, *PSERVER_INFO_1518;

#endif


#ifndef SERVER_INFO_1520_DEFINED
#define SERVER_INFO_1520_DEFINED 1

typedef struct _SERVER_INFO_1520 {
    UINT32 sv1520_maxcopyreadlen;
} SERVER_INFO_1520, *PSERVER_INFO_1520;

#endif


#ifndef SERVER_INFO_1521_DEFINED
#define SERVER_INFO_1521_DEFINED 1

typedef struct _SERVER_INFO_1521 {
    UINT32 sv1521_maxcopywritelen;
} SERVER_INFO_1521, *PSERVER_INFO_1521;

#endif


#ifndef SERVER_INFO_1522_DEFINED
#define SERVER_INFO_1522_DEFINED 1

typedef struct _SERVER_INFO_1522 {
    UINT32 sv1522_minkeepsearch;
} SERVER_INFO_1522, *PSERVER_INFO_1522;

#endif


#ifndef SERVER_INFO_1523_DEFINED
#define SERVER_INFO_1523_DEFINED 1

typedef struct _SERVER_INFO_1523 {
    UINT32 sv1523_maxkeepsearch;
} SERVER_INFO_1523, *PSERVER_INFO_1523;

#endif


#ifndef SERVER_INFO_1524_DEFINED
#define SERVER_INFO_1524_DEFINED 1

typedef struct _SERVER_INFO_1524 {
    UINT32 sv1524_minkeepcomplsearch;
} SERVER_INFO_1524, *PSERVER_INFO_1524;

#endif


#ifndef SERVER_INFO_1525_DEFINED
#define SERVER_INFO_1525_DEFINED 1

typedef struct _SERVER_INFO_1525 {
    UINT32 sv1525_maxkeepcomplsearch;
} SERVER_INFO_1525, *PSERVER_INFO_1525;

#endif

#ifndef SERVER_INFO_1528_DEFINED
#define SERVER_INFO_1528_DEFINED 1

typedef struct _SERVER_INFO_1528 {
    UINT32 sv1528_scavtimeout;
} SERVER_INFO_1528, *PSERVER_INFO_1528;

#endif


#ifndef SERVER_INFO_1529_DEFINED
#define SERVER_INFO_1529_DEFINED 1

typedef struct _SERVER_INFO_1529 {
    UINT32 sv1529_minrcvqueue;
} SERVER_INFO_1529, *PSERVER_INFO_1529;

#endif


#ifndef SERVER_INFO_1530_DEFINED
#define SERVER_INFO_1530_DEFINED 1

typedef struct _SERVER_INFO_1530 {
    UINT32 sv1530_minfreeworkitems;
} SERVER_INFO_1530, *PSERVER_INFO_1530;

#endif


#ifndef SERVER_INFO_1533_DEFINED
#define SERVER_INFO_1533_DEFINED 1

typedef struct _SERVER_INFO_1533 {
    UINT32 sv1533_maxmpxct;
} SERVER_INFO_1533, *PSERVER_INFO_1533;

#endif


#ifndef SERVER_INFO_1534_DEFINED
#define SERVER_INFO_1534_DEFINED 1

typedef struct _SERVER_INFO_1534 {
    UINT32 sv1534_oplockbreakwait;
} SERVER_INFO_1534, *PSERVER_INFO_1534;

#endif


#ifndef SERVER_INFO_1535_DEFINED
#define SERVER_INFO_1535_DEFINED 1

typedef struct _SERVER_INFO_1535 {
    UINT32 sv1535_oplockbreakresponsewait;
} SERVER_INFO_1535, *PSERVER_INFO_1535;

#endif


#ifndef SERVER_INFO_1536_DEFINED
#define SERVER_INFO_1536_DEFINED 1

typedef struct _SERVER_INFO_1536 {
    UINT32 sv1536_enableoplocks;
} SERVER_INFO_1536, *PSERVER_INFO_1536;

#endif


#ifndef SERVER_INFO_1537_DEFINED
#define SERVER_INFO_1537_DEFINED 1

typedef struct _SERVER_INFO_1537 {
    UINT32 sv1537_enableoplockforceclose;
} SERVER_INFO_1537, *PSERVER_INFO_1537;

#endif

#ifndef SERVER_INFO_1538_DEFINED
#define SERVER_INFO_1538_DEFINED 1

typedef struct _SERVER_INFO_1538 {
    UINT32 sv1538_enablefcbopens;
} SERVER_INFO_1538, *PSERVER_INFO_1538;

#endif


#ifndef SERVER_INFO_1539_DEFINED
#define SERVER_INFO_1539_DEFINED 1

typedef struct _SERVER_INFO_1539 {
    UINT32 sv1539_enableraw;
} SERVER_INFO_1539, *PSERVER_INFO_1539;

#endif


#ifndef SERVER_INFO_1540_DEFINED
#define SERVER_INFO_1540_DEFINED 1

typedef struct _SERVER_INFO_1540 {
    UINT32 sv1540_enablesharednetdrives;
} SERVER_INFO_1540, *PSERVER_INFO_1540;

#endif


#ifndef SERVER_INFO_1541_DEFINED
#define SERVER_INFO_1541_DEFINED 1

typedef struct _SERVER_INFO_1541 {
    UINT32 sv1541_minfreeconnections;
} SERVER_INFO_1541, *PSERVER_INFO_1541;

#endif


#ifndef SERVER_INFO_1542_DEFINED
#define SERVER_INFO_1542_DEFINED 1

typedef struct _SERVER_INFO_1542 {
    UINT32 sv1542_maxfreeconnections;
} SERVER_INFO_1542, *PSERVER_INFO_1542;

#endif


#ifndef SERVER_INFO_1543_DEFINED
#define SERVER_INFO_1543_DEFINED 1

typedef struct _SERVER_INFO_1543 {
    UINT32 sv1543_initsesstable;
} SERVER_INFO_1543, *PSERVER_INFO_1543;

#endif


#ifndef SERVER_INFO_1544_DEFINED
#define SERVER_INFO_1544_DEFINED 1

typedef struct _SERVER_INFO_1544 {
    UINT32 sv1544_initconntable;
} SERVER_INFO_1544, *PSERVER_INFO_1544;

#endif


#ifndef SERVER_INFO_1545_DEFINED
#define SERVER_INFO_1545_DEFINED 1

typedef struct _SERVER_INFO_1545 {
    UINT32 sv1545_initfiletable;
} SERVER_INFO_1545, *PSERVER_INFO_1545;

#endif


#ifndef SERVER_INFO_1546_DEFINED
#define SERVER_INFO_1546_DEFINED 1

typedef struct _SERVER_INFO_1546 {
    UINT32 sv1546_initsearchtable;
} SERVER_INFO_1546, *PSERVER_INFO_1546;

#endif


#ifndef SERVER_INFO_1547_DEFINED
#define SERVER_INFO_1547_DEFINED 1

typedef struct _SERVER_INFO_1547 {
    UINT32 sv1547_alertsched;
} SERVER_INFO_1547, *PSERVER_INFO_1547;

#endif


#ifndef SERVER_INFO_1548_DEFINED
#define SERVER_INFO_1548_DEFINED 1

typedef struct _SERVER_INFO_1548 {
    UINT32 sv1548_errorthreshold;
} SERVER_INFO_1548, *PSERVER_INFO_1548;

#endif


#ifndef SERVER_INFO_1549_DEFINED
#define SERVER_INFO_1549_DEFINED 1

typedef struct _SERVER_INFO_1549 {
    UINT32 sv1549_networkerrorthreshold;
} SERVER_INFO_1549, *PSERVER_INFO_1549;

#endif


#ifndef SERVER_INFO_1550_DEFINED
#define SERVER_INFO_1550_DEFINED 1

typedef struct _SERVER_INFO_1550 {
    UINT32 sv1550_diskspacethreshold;
} SERVER_INFO_1550, *PSERVER_INFO_1550;

#endif


#ifndef SERVER_INFO_1552_DEFINED
#define SERVER_INFO_1552_DEFINED 1

typedef struct _SERVER_INFO_1552 {
    UINT32 sv1552_maxlinkdelay;
} SERVER_INFO_1552, *PSERVER_INFO_1552;

#endif


#ifndef SERVER_INFO_1553_DEFINED
#define SERVER_INFO_1553_DEFINED 1

typedef struct _SERVER_INFO_1553 {
    UINT32 sv1553_minlinkthroughput;
} SERVER_INFO_1553, *PSERVER_INFO_1553;

#endif


#ifndef SERVER_INFO_1554_DEFINED
#define SERVER_INFO_1554_DEFINED 1

typedef struct _SERVER_INFO_1554 {
    UINT32 sv1554_linkinfovalidtime;
} SERVER_INFO_1554, *PSERVER_INFO_1554;

#endif


#ifndef SERVER_INFO_1555_DEFINED
#define SERVER_INFO_1555_DEFINED 1

typedef struct _SERVER_INFO_1555 {
    UINT32 sv1555_scavqosinfoupdatetime;
} SERVER_INFO_1555, *PSERVER_INFO_1555;

#endif


#ifndef SERVER_INFO_1556_DEFINED
#define SERVER_INFO_1556_DEFINED 1

typedef struct _SERVER_INFO_1556 {
    UINT32 sv1556_maxworkitemidletime;
} SERVER_INFO_1556, *PSERVER_INFO_1556;

#endif


#ifndef _DCE_IDL_

typedef union {
    SERVER_INFO_100 *info100;
    SERVER_INFO_101 *info101;
    SERVER_INFO_102 *info102;
    SERVER_INFO_402 *info402;
    SERVER_INFO_403 *info403;
    SERVER_INFO_502 *info502;
    SERVER_INFO_503 *info503;
    SERVER_INFO_599 *info599;
    SERVER_INFO_1005 *info1005;
    SERVER_INFO_1010 *info1010;
    SERVER_INFO_1016 *info1016;
    SERVER_INFO_1017 *info1017;
    SERVER_INFO_1018 *info1018;
    SERVER_INFO_1107 *info1107;
    SERVER_INFO_1501 *info1501;
    SERVER_INFO_1502 *info1502;
    SERVER_INFO_1503 *info1503;
    SERVER_INFO_1506 *info1506;
    SERVER_INFO_1509 *info1509;
    SERVER_INFO_1510 *info1510;
    SERVER_INFO_1511 *info1511;
    SERVER_INFO_1512 *info1512;
    SERVER_INFO_1513 *info1513;
    SERVER_INFO_1514 *info1514;
    SERVER_INFO_1515 *info1515;
    SERVER_INFO_1516 *info1516;
    SERVER_INFO_1518 *info1518;
    SERVER_INFO_1520 *info1520;
    SERVER_INFO_1521 *info1521;
    SERVER_INFO_1522 *info1522;
    SERVER_INFO_1523 *info1523;
    SERVER_INFO_1524 *info1524;
    SERVER_INFO_1525 *info1525;
    SERVER_INFO_1528 *info1528;
    SERVER_INFO_1529 *info1529;
    SERVER_INFO_1530 *info1530;
    SERVER_INFO_1533 *info1533;
    SERVER_INFO_1534 *info1534;
    SERVER_INFO_1535 *info1535;
    SERVER_INFO_1536 *info1536;
    SERVER_INFO_1537 *info1537;
    SERVER_INFO_1538 *info1538;
    SERVER_INFO_1539 *info1539;
    SERVER_INFO_1540 *info1540;
    SERVER_INFO_1541 *info1541;
    SERVER_INFO_1542 *info1542;
    SERVER_INFO_1543 *info1543;
    SERVER_INFO_1544 *info1544;
    SERVER_INFO_1545 *info1545;
    SERVER_INFO_1546 *info1546;
    SERVER_INFO_1547 *info1547;
    SERVER_INFO_1548 *info1548;
    SERVER_INFO_1549 *info1549;
    SERVER_INFO_1550 *info1550;
    SERVER_INFO_1552 *info1552;
    SERVER_INFO_1553 *info1553;
    SERVER_INFO_1554 *info1554;
    SERVER_INFO_1555 *info1555;
    SERVER_INFO_1556 *info1556;
} nfssvc_NetNfsInfo;

#endif

#ifndef TIME_OF_DAY_INFO_DEFINED
#define TIME_OF_DAY_INFO_DEFINED 1

typedef struct _TIME_OF_DAY_INFO {
    UINT32 tod_elapsedt; /* time(NULL) */
    UINT32 tod_msecs; /* milliseconds till system reboot (uptime) */
    UINT32 tod_hours;
    UINT32 tod_mins;
    UINT32 tod_secs;
    UINT32 tod_hunds;
    INT32  tod_timezone; /* in minutes */
    UINT32 tod_tinterval; /* clock tick interval in 0.0001 second units; 310 on windows */
    UINT32 tod_day;
    UINT32 tod_month;
    UINT32 tod_year;
    UINT32 tod_weekday;
} TIME_OF_DAY_INFO, *PTIME_OF_DAY_INFO;

#endif /* TIME_OF_DAY_INFO_DEFINED */

#if !defined(_DCE_IDL_)

/*
 * Error codes
 */
#define NFSSVC_ERROR_SUCCESS                   0x0000
#define NFSSVC_ERROR_INVALID_CONFIG_PATH       0x9400 // 37888
#define NFSSVC_ERROR_INVALID_PREFIX_PATH       0x9401 // 37889
#define NFSSVC_ERROR_INSUFFICIENT_BUFFER       0x9402 // 37890
#define NFSSVC_ERROR_OUT_OF_MEMORY             0x9403 // 37891
#define NFSSVC_ERROR_INVALID_MESSAGE           0x9404 // 37892
#define NFSSVC_ERROR_UNEXPECTED_MESSAGE        0x9405 // 37893
#define NFSSVC_ERROR_NO_SUCH_USER              0x9406 // 37894
#define NFSSVC_ERROR_DATA_ERROR                0x9407 // 37895
#define NFSSVC_ERROR_NOT_IMPLEMENTED           0x9408 // 37896
#define NFSSVC_ERROR_NO_CONTEXT_ITEM           0x9409 // 37897
#define NFSSVC_ERROR_NO_SUCH_GROUP             0x940A // 37898
#define NFSSVC_ERROR_REGEX_COMPILE_FAILED      0x940B // 37899
#define NFSSVC_ERROR_NSS_EDIT_FAILED           0x940C // 37900
#define NFSSVC_ERROR_NO_HANDLER                0x940D // 37901
#define NFSSVC_ERROR_INTERNAL                  0x940E // 37902
#define NFSSVC_ERROR_NOT_HANDLED               0x940F // 37903
#define NFSSVC_ERROR_UNEXPECTED_DB_RESULT      0x9410 // 37904
#define NFSSVC_ERROR_INVALID_PARAMETER         0x9411 // 37905
#define NFSSVC_ERROR_LOAD_LIBRARY_FAILED       0x9412 // 37906
#define NFSSVC_ERROR_LOOKUP_SYMBOL_FAILED      0x9413 // 37907
#define NFSSVC_ERROR_INVALID_EVENTLOG          0x9414 // 37908
#define NFSSVC_ERROR_INVALID_CONFIG            0x9415 // 37909
#define NFSSVC_ERROR_STRING_CONV_FAILED        0x9416 // 37910
#define NFSSVC_ERROR_INVALID_DB_HANDLE         0x9417 // 37911
#define NFSSVC_ERROR_FAILED_CONVERT_TIME       0x9418 // 37912
#define NFSSVC_ERROR_RPC_EXCEPTION_UPON_RPC_BINDING 0x9419 // 37913
#define NFSSVC_ERROR_RPC_EXCEPTION_UPON_OPEN   0x941A // 37914
#define NFSSVC_ERROR_RPC_EXCEPTION_UPON_CLOSE  0x941B // 37915
#define NFSSVC_ERROR_RPC_EXCEPTION_UPON_COUNT  0x941C // 37916
#define NFSSVC_ERROR_RPC_EXCEPTION_UPON_READ   0x941D // 37917
#define NFSSVC_ERROR_RPC_EXCEPTION_UPON_WRITE  0x941E // 37918
#define NFSSVC_ERROR_RPC_EXCEPTION_UPON_CLEAR  0x941F // 37919
#define NFSSVC_ERROR_RPC_EXCEPTION_UPON_DELETE 0x9420 // 37920
#define NFSSVC_ERROR_RPC_EXCEPTION_UPON_REGISTER 0x9421 // 37921
#define NFSSVC_ERROR_RPC_EXCEPTION_UPON_UNREGISTER 0x9422 // 37922
#define NFSSVC_ERROR_RPC_EXCEPTION_UPON_LISTEN 0x9423 // 37923
#define NFSSVC_ERROR_RPC_EXCEPTION             0x9424 // 37924
#define NFSSVC_ERROR_ACCESS_DENIED             0x9425 // 37925
#define NFSSVC_ERROR_SENTINEL                  0x9426 // 37926

#ifndef NET_API_STATUS_DEFINED
typedef WINERR NET_API_STATUS;

#define NET_API_STATUS_DEFINED
#endif

typedef struct _NFSSVC_CONTEXT* PNFSSVC_CONTEXT;

NET_API_STATUS
NfsSvcCreateContext(
    IN  PCWSTR           pwszHostname,
    OUT PNFSSVC_CONTEXT* ppContext
    );

NET_API_STATUS
NetrServerGetInfo(
    PNFSSVC_CONTEXT pContext,        /* IN             */
    PCWSTR          pwszServername,  /* IN    OPTIONAL */
    DWORD           dwInfoLevel,     /* IN             */
    PBYTE*          ppBuffer         /*    OUT         */
    );

NET_API_STATUS
NetrServerSetInfo(
    PNFSSVC_CONTEXT pContext,        /* IN              */
    PCWSTR          pwszServername,  /* IN     OPTIONAL */
    DWORD           dwInfoLevel,     /* IN              */
    PBYTE           pBuffer,         /* IN              */
    PDWORD          pdwParmError     /*    OUT OPTIONAL */
    );

NET_API_STATUS
NetrShareEnum(
    PNFSSVC_CONTEXT pContext,        /* IN              */
    PCWSTR          pwszServername,  /* IN     OPTIONAL */
    DWORD           dwInfoLevel,     /* IN              */
    PBYTE*          ppBuffer,        /*    OUT          */
    DWORD           dwPrefmaxLen,    /* IN              */
    PDWORD          pdwEntriesRead,  /*    OUT          */
    PDWORD          pdwTotalEntries, /*    OUT          */
    PDWORD          pdwResumeHandle  /* IN OUT OPTIONAL */
    );

NET_API_STATUS
NetrShareGetInfo(
    PNFSSVC_CONTEXT pContext,        /* IN              */
    PCWSTR          pwszServername,  /* IN     OPTIONAL */
    PCWSTR          pwszNetname,     /* IN              */
    DWORD           dwInfoLevel,     /* IN              */
    PBYTE*          ppBuffer         /*    OUT          */
    );

NET_API_STATUS
NetrShareSetInfo(
    PNFSSVC_CONTEXT pContext,        /* IN              */
    PCWSTR          pwszServername,  /* IN     OPTIONAL */
    PCWSTR          pwszNetname,     /* IN              */
    DWORD           dwInfoLevel,     /* IN              */
    PBYTE           pBuffer,         /* IN              */
    PDWORD          pdwParmError     /*    OUT          */
    );

NET_API_STATUS
NetrShareAdd(
    PNFSSVC_CONTEXT pContext,        /* IN              */
    PCWSTR          pwszServername,  /* IN     OPTIONAL */
    DWORD           dwInfoLevel,     /* IN              */
    PBYTE           pBuffer,         /* IN              */
    PDWORD          pdwParmError     /*    OUT          */
    );

NET_API_STATUS
NetrShareDel(
    PNFSSVC_CONTEXT pContext,        /* IN              */
    PCWSTR          pwszServername,  /* IN     OPTIONAL */
    PCWSTR          pwszNetname,     /* IN              */
    DWORD           dwReserved       /* IN              */
    );

NET_API_STATUS
NetrSessionEnum(
    PNFSSVC_CONTEXT pContext,          /* IN              */
    PCWSTR          pwszServername,    /* IN     OPTIONAL */
    PCWSTR          pwszUncClientname, /* IN     OPTIONAL */
    PCWSTR          pwszUsername,      /* IN     OPTIONAL */
    DWORD           dwInfoLevel,       /* IN              */
    PBYTE*          ppBuffer,          /*    OUT          */
    DWORD           dwPrefmaxLen,      /* IN              */
    PDWORD          pdwEntriesRead,    /*    OUT          */
    PDWORD          pdwTotalEntries,   /*    OUT          */
    PDWORD          pdwResumeHandle    /* IN OUT OPTIONAL */
    );

NET_API_STATUS
NetrSessionDel(
    PNFSSVC_CONTEXT pContext,          /* IN              */
    PCWSTR          pwszServername,    /* IN     OPTIONAL */
    PCWSTR          pwszUncClientname, /* IN     OPTIONAL */
    PCWSTR          pwszUsername       /* IN     OPTIONAL */
    );

NET_API_STATUS
NetrConnectionEnum(
    PNFSSVC_CONTEXT pContext,          /* IN              */
    PCWSTR          pwszServername,    /* IN     OPTIONAL */
    PCWSTR          pwszQualifier,     /* IN              */
    DWORD           dwInfoLevel,       /* IN              */
    PBYTE*          ppBuffer,          /*    OUT          */
    DWORD           dwPrefmaxlen,      /* IN              */
    PDWORD          pdwEntriesRead,    /*    OUT          */
    PDWORD          pdwTotalEntries,   /*    OUT          */
    PDWORD          pdwResumeHandle    /* IN OUT OPTIONAL */
    );

NET_API_STATUS
NetrFileEnum(
    PNFSSVC_CONTEXT pContext,          /* IN              */
    PCWSTR          pwszServername,    /* IN    OPTIONAL  */
    PCWSTR          pwszBasepath,      /* IN    OPTIONAL  */
    PCWSTR          pwszUsername,      /* IN    OPTIONAL  */
    DWORD           dwInfoLevel,       /* IN              */
    PBYTE*          ppBuffer,          /*    OUT          */
    DWORD           dwPrefmaxlen,      /* IN              */
    PDWORD          pwdEntriesRead,    /*    OUT          */
    PDWORD          pdwTotalEntries,   /*    OUT          */
    PDWORD          pdwResumeHandle    /* IN OUT OPTIONAL */
    );

NET_API_STATUS
NetrFileClose(
    PNFSSVC_CONTEXT pContext,          /* IN              */
    PCWSTR          pwszServername,    /* IN    OPTIONAL  */
    DWORD           dwFileId           /* IN              */
    );

NET_API_STATUS
NetrRemoteTOD(
    PNFSSVC_CONTEXT pContext,          /* IN              */
    PCWSTR          pwszServername,    /* IN    OPTIONAL  */
    PBYTE*          ppBuffer           /*    OUT          */
    );

NET_API_STATUS
NfsSvcCloseContext(
    IN  PNFSSVC_CONTEXT pContext
    );

NET_API_STATUS
NfsSvcInitMemory(
    void
    );


NET_API_STATUS
NfsSvcDestroyMemory(
    void
    );


NET_API_STATUS
NfsSvcFreeMemory(
    void *ptr
    );

#endif /* !defined(_DCE_IDL_) */


#endif /* _NFSSVC_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
