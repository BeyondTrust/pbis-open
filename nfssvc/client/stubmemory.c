/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#include "includes.h"


static void NfsSvcClearCONNECTION_INFO_1(PCONNECTION_INFO_1 info)
{
    if (!info) {
        return;
    }

    NFSSVC_SAFE_FREE(info->coni1_username);
    NFSSVC_SAFE_FREE(info->coni1_netname);
}

void NfsSvcClearNetConnCtr(UINT32 level, nfssvc_NetConnCtr *ctr)
{
    int i;

    if (!ctr) {
        return;
    }

    switch (level) {
    case 0:
        if (!ctr->ctr0) {
            break;
        }

        NFSSVC_SAFE_FREE(ctr->ctr0->array);
        NFSSVC_SAFE_FREE(ctr->ctr0);
        break;
    case 1:
        if (!ctr->ctr1) {
            break;
        }

        for (i=0; i < ctr->ctr1->count; i++) {
             NfsSvcClearCONNECTION_INFO_1(&ctr->ctr1->array[i]);
        }
        NFSSVC_SAFE_FREE(ctr->ctr1->array);
        NFSSVC_SAFE_FREE(ctr->ctr1);
        break;
    }
}

static void NfsSvcClearFILE_INFO_3(PFILE_INFO_3 info)
{
    if (!info) {
        return;
    }

    NFSSVC_SAFE_FREE(info->fi3_path_name);
    NFSSVC_SAFE_FREE(info->fi3_username);
}

void NfsSvcClearNetFileCtr(UINT32 level, nfssvc_NetFileCtr *ctr)
{
    int i;

    if (!ctr) {
        return;
    }

    switch (level) {
    case 2:
        if (!ctr->ctr2) {
            break;
        }

        NFSSVC_SAFE_FREE(ctr->ctr2->array);
        NFSSVC_SAFE_FREE(ctr->ctr2);
        break;
    case 3:
        if (!ctr->ctr3) {
            break;
        }

        for (i=0; i < ctr->ctr3->count; i++) {
             NfsSvcClearFILE_INFO_3(&ctr->ctr3->array[i]);
        }
        NFSSVC_SAFE_FREE(ctr->ctr3->array);
        NFSSVC_SAFE_FREE(ctr->ctr3);
        break;
    }
}

void NfsSvcClearNetFileInfo(UINT32 level, nfssvc_NetFileInfo *info)
{
    if (!info) {
        return;
    }

    switch (level) {
    case 2:
        if (!info->info2) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info2);
        break;
    case 3:
        if (!info->info3) {
            break;
        }

        NfsSvcClearFILE_INFO_3(info->info3);
        NFSSVC_SAFE_FREE(info->info3);
        break;
    }
}

static void NfsSvcClearSESSION_INFO_0(PSESSION_INFO_0 info)
{
    if (!info) {
        return;
    }

    NFSSVC_SAFE_FREE(info->sesi0_cname);
}

static void NfsSvcClearSESSION_INFO_1(PSESSION_INFO_1 info)
{
    if (!info) {
        return;
    }

    NFSSVC_SAFE_FREE(info->sesi1_cname);
    NFSSVC_SAFE_FREE(info->sesi1_username);
}

static void NfsSvcClearSESSION_INFO_2(PSESSION_INFO_2 info)
{
    if (!info) {
        return;
    }

    NFSSVC_SAFE_FREE(info->sesi2_cname);
    NFSSVC_SAFE_FREE(info->sesi2_username);
    NFSSVC_SAFE_FREE(info->sesi2_cltype_name);
}

static void NfsSvcClearSESSION_INFO_10(PSESSION_INFO_10 info)
{
    if (!info) {
        return;
    }

    NFSSVC_SAFE_FREE(info->sesi10_cname);
    NFSSVC_SAFE_FREE(info->sesi10_username);
}

static void NfsSvcClearSESSION_INFO_502(PSESSION_INFO_502 info)
{
    if (!info) {
        return;
    }

    NFSSVC_SAFE_FREE(info->sesi502_cname);
    NFSSVC_SAFE_FREE(info->sesi502_username);
    NFSSVC_SAFE_FREE(info->sesi502_cltype_name);
    NFSSVC_SAFE_FREE(info->sesi502_transport);
}

void NfsSvcClearNetSessCtr(UINT32 level, nfssvc_NetSessCtr *ctr)
{
    int i;

    if (!ctr) {
        return;
    }

    switch (level) {
    case 0:
        if (!ctr->ctr0) {
            break;
        }

        for (i=0; i < ctr->ctr0->count; i++) {
             NfsSvcClearSESSION_INFO_0(&ctr->ctr0->array[i]);
        }
        NFSSVC_SAFE_FREE(ctr->ctr0->array);
        NFSSVC_SAFE_FREE(ctr->ctr0);
        break;
    case 1:
        if (!ctr->ctr1) {
            break;
        }

        for (i=0; i < ctr->ctr1->count; i++) {
             NfsSvcClearSESSION_INFO_1(&ctr->ctr1->array[i]);
        }
        NFSSVC_SAFE_FREE(ctr->ctr1->array);
        NFSSVC_SAFE_FREE(ctr->ctr1);
        break;
    case 2:
        if (!ctr->ctr2) {
            break;
        }

        for (i=0; i < ctr->ctr2->count; i++) {
             NfsSvcClearSESSION_INFO_2(&ctr->ctr2->array[i]);
        }
        NFSSVC_SAFE_FREE(ctr->ctr2->array);
        NFSSVC_SAFE_FREE(ctr->ctr2);
        break;
    case 10:
        if (!ctr->ctr10) {
            break;
        }

        for (i=0; i < ctr->ctr10->count; i++) {
             NfsSvcClearSESSION_INFO_10(&ctr->ctr10->array[i]);
        }
        NFSSVC_SAFE_FREE(ctr->ctr10->array);
        NFSSVC_SAFE_FREE(ctr->ctr10);
        break;
    case 502:
        if (!ctr->ctr502) {
            break;
        }

        for (i=0; i < ctr->ctr502->count; i++) {
             NfsSvcClearSESSION_INFO_502(&ctr->ctr502->array[i]);
        }
        NFSSVC_SAFE_FREE(ctr->ctr502->array);
        NFSSVC_SAFE_FREE(ctr->ctr502);
        break;
    }
}

static void NfsSvcClearSHARE_INFO_0(PSHARE_INFO_0 info)
{
    if (!info) {
        return;
    }

    NFSSVC_SAFE_FREE(info->shi0_netname);
}

static void NfsSvcClearSHARE_INFO_1(PSHARE_INFO_1 info)
{
    if (!info) {
        return;
    }

    NFSSVC_SAFE_FREE(info->shi1_netname);
    NFSSVC_SAFE_FREE(info->shi1_remark);
}

static void NfsSvcClearSHARE_INFO_2(PSHARE_INFO_2 info)
{
    if (!info) {
        return;
    }

    NFSSVC_SAFE_FREE(info->shi2_netname);
    NFSSVC_SAFE_FREE(info->shi2_remark);
    NFSSVC_SAFE_FREE(info->shi2_path);
    NFSSVC_SAFE_FREE(info->shi2_password);
}

static void NfsSvcClearSHARE_INFO_501(PSHARE_INFO_501 info)
{
    if (!info) {
        return;
    }

    NFSSVC_SAFE_FREE(info->shi501_netname);
    NFSSVC_SAFE_FREE(info->shi501_remark);
}

static void NfsSvcClearSHARE_INFO_502_I(PSHARE_INFO_502_I info)
{
    if (!info) {
        return;
    }

    NFSSVC_SAFE_FREE(info->shi502_netname);
    NFSSVC_SAFE_FREE(info->shi502_remark);
    NFSSVC_SAFE_FREE(info->shi502_path);
    NFSSVC_SAFE_FREE(info->shi502_password);
    NFSSVC_SAFE_FREE(info->shi502_security_descriptor);
}

void NfsSvcClearNetShareCtr(UINT32 level, nfssvc_NetShareCtr *ctr)
{
    int i;

    if (!ctr) {
        return;
    }

    switch (level) {
    case 0:
        if (!ctr->ctr0) {
            break;
        }

        for (i=0; i < ctr->ctr0->count; i++) {
             NfsSvcClearSHARE_INFO_0(&ctr->ctr0->array[i]);
        }
        NFSSVC_SAFE_FREE(ctr->ctr0->array);
        break;
    case 1:
        if (!ctr->ctr1) {
            break;
        }

        for (i=0; i < ctr->ctr1->count; i++) {
             NfsSvcClearSHARE_INFO_1(&ctr->ctr1->array[i]);
        }
        NFSSVC_SAFE_FREE(ctr->ctr1->array);
        break;
    case 2:
        if (!ctr->ctr2) {
            break;
        }

        for (i=0; i < ctr->ctr2->count; i++) {
             NfsSvcClearSHARE_INFO_2(&ctr->ctr2->array[i]);
        }
        NFSSVC_SAFE_FREE(ctr->ctr2->array);
        break;
    case 501:
        if (!ctr->ctr501) {
            break;
        }

        for (i=0; i < ctr->ctr501->count; i++) {
             NfsSvcClearSHARE_INFO_501(&ctr->ctr501->array[i]);
        }
        NFSSVC_SAFE_FREE(ctr->ctr501->array);
        break;
    case 502:
        if (!ctr->ctr502) {
            break;
        }

        for (i=0; i < ctr->ctr502->count; i++) {
             NfsSvcClearSHARE_INFO_502_I(&ctr->ctr502->array[i]);
        }
        NFSSVC_SAFE_FREE(ctr->ctr502->array);
        break;
    }
}

void NfsSvcClearNetShareInfo(UINT32 level, nfssvc_NetShareInfo *info)
{
    if (!info) {
        return;
    }

    switch (level) {
    case 0:
        if (!info->info0) {
            break;
        }

        NfsSvcClearSHARE_INFO_0(info->info0);
        NFSSVC_SAFE_FREE(info->info0);
        break;
    case 1:
        if (!info->info1) {
            break;
        }

        NfsSvcClearSHARE_INFO_1(info->info1);
        NFSSVC_SAFE_FREE(info->info1);
        break;
    case 2:
        if (!info->info2) {
            break;
        }

        NfsSvcClearSHARE_INFO_2(info->info2);
        NFSSVC_SAFE_FREE(info->info2);
        break;
    case 501:
        if (!info->info501) {
            break;
        }

        NfsSvcClearSHARE_INFO_501(info->info501);
        NFSSVC_SAFE_FREE(info->info501);
        break;
    case 502:
        if (!info->info502) {
            break;
        }

        NfsSvcClearSHARE_INFO_502_I(info->info502);
        NFSSVC_SAFE_FREE(info->info502);
        break;
    }
}

static void NfsSvcClearSERVER_INFO_100(PSERVER_INFO_100 info)
{
    if (!info) {
        return;
    }

    NFSSVC_SAFE_FREE(info->sv100_name);
}

static void NfsSvcClearSERVER_INFO_101(PSERVER_INFO_101 info)
{
    if (!info) {
        return;
    }

    NFSSVC_SAFE_FREE(info->sv101_name);
    NFSSVC_SAFE_FREE(info->sv101_comment);
}

static void NfsSvcClearSERVER_INFO_102(PSERVER_INFO_102 info)
{
    if (!info) {
        return;
    }

    NFSSVC_SAFE_FREE(info->sv102_name);
    NFSSVC_SAFE_FREE(info->sv102_comment);
    NFSSVC_SAFE_FREE(info->sv102_userpath);
}

static void NfsSvcClearSERVER_INFO_402(PSERVER_INFO_402 info)
{
    if (!info) {
        return;
    }

    NFSSVC_SAFE_FREE(info->sv402_alerts);
    NFSSVC_SAFE_FREE(info->sv402_guestacct);
    NFSSVC_SAFE_FREE(info->sv402_srvheuristics);
}

static void NfsSvcClearSERVER_INFO_403(PSERVER_INFO_403 info)
{
    if (!info) {
        return;
    }

    NFSSVC_SAFE_FREE(info->sv403_alerts);
    NFSSVC_SAFE_FREE(info->sv403_guestacct);
    NFSSVC_SAFE_FREE(info->sv403_srvheuristics);
    NFSSVC_SAFE_FREE(info->sv403_autopath);
}

static void NfsSvcClearSERVER_INFO_503(PSERVER_INFO_503 info)
{
    if (!info) {
        return;
    }

    NFSSVC_SAFE_FREE(info->sv503_domain);
}

static void NfsSvcClearSERVER_INFO_599(PSERVER_INFO_599 info)
{
    if (!info) {
        return;
    }

    NFSSVC_SAFE_FREE(info->sv599_domain);
}

static void NfsSvcClearSERVER_INFO_1005(PSERVER_INFO_1005 info)
{
    if (!info) {
        return;
    }

    NFSSVC_SAFE_FREE(info->sv1005_comment);
}

void NfsSvcClearNetNfsInfo(UINT32 level, nfssvc_NetNfsInfo *info)
{
    if (!info) {
        return;
    }

    switch (level) {
    case 100:
        if (!info->info100) {
            break;
        }

        NfsSvcClearSERVER_INFO_100(info->info100);
        NFSSVC_SAFE_FREE(info->info100);
        break;
    case 101:
        if (!info->info101) {
            break;
        }

        NfsSvcClearSERVER_INFO_101(info->info101);
        NFSSVC_SAFE_FREE(info->info101);
        break;
    case 102:
        if (!info->info102) {
            break;
        }

        NfsSvcClearSERVER_INFO_102(info->info102);
        NFSSVC_SAFE_FREE(info->info102);
        break;
    case 402:
        if (!info->info402) {
            break;
        }

        NfsSvcClearSERVER_INFO_402(info->info402);
        NFSSVC_SAFE_FREE(info->info402);
        break;
    case 403:
        if (!info->info403) {
            break;
        }

        NfsSvcClearSERVER_INFO_403(info->info403);
        NFSSVC_SAFE_FREE(info->info403);
        break;
    case 502:
        if (!info->info502) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info502);
        break;
    case 503:
        if (!info->info503) {
            break;
        }

        NfsSvcClearSERVER_INFO_503(info->info503);
        NFSSVC_SAFE_FREE(info->info503);
        break;
    case 599:
        if (!info->info599) {
            break;
        }

        NfsSvcClearSERVER_INFO_599(info->info599);
        NFSSVC_SAFE_FREE(info->info599);
        break;
    case 1005:
        if (!info->info1005) {
            break;
        }

        NfsSvcClearSERVER_INFO_1005(info->info1005);
        NFSSVC_SAFE_FREE(info->info1005);
        break;
    case 1010:
        if (!info->info1010) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1010);
        break;
    case 1016:
        if (info->info1016) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1016);
        break;
    case 1017:
        if (info->info1017) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1017);
        break;
    case 1018:
        if (info->info1018) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1018);
        break;
    case 1107:
        if (info->info1107) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1107);
        break;
    case 1501:
        if (info->info1501) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1501);
        break;
    case 1502:
        if (info->info1502) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1502);
        break;
    case 1503:
        if (info->info1503) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1503);
        break;
    case 1506:
        if (info->info1506) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1506);
        break;
    case 1509:
        if (info->info1509) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1509);
        break;
    case 1510:
        if (info->info1510) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1510);
        break;
    case 1511:
        if (info->info1511) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1511);
        break;
    case 1512:
        if (info->info1512) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1512);
        break;
    case 1513:
        if (info->info1513) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1513);
        break;
    case 1514:
        if (info->info1514) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1514);
        break;
    case 1515:
        if (info->info1515) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1515);
        break;
    case 1516:
        if (info->info1516) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1516);
        break;
    case 1518:
        if (info->info1518) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1518);
        break;
    case 1520:
        if (info->info1520) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1520);
        break;
    case 1521:
        if (info->info1521) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1521);
        break;
    case 1522:
        if (info->info1522) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1522);
        break;
    case 1523:
        if (info->info1523) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1523);
        break;
    case 1524:
        if (info->info1524) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1524);
        break;
    case 1525:
        if (info->info1525) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1525);
        break;
    case 1528:
        if (info->info1528) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1528);
        break;
    case 1529:
        if (info->info1529) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1529);
        break;
    case 1530:
        if (info->info1530) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1530);
        break;
    case 1533:
        if (info->info1533) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1533);
        break;
    case 1534:
        if (info->info1534) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1534);
        break;
    case 1535:
        if (info->info1535) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1535);
        break;
    case 1536:
        if (info->info1536) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1536);
        break;
    case 1537:
        if (info->info1537) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1537);
        break;
    case 1538:
        if (info->info1538) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1538);
        break;
    case 1539:
        if (info->info1539) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1539);
        break;
    case 1540:
        if (info->info1540) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1540);
        break;
    case 1541:
        if (info->info1541) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1541);
        break;
    case 1542:
        if (info->info1542) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1542);
        break;
    case 1543:
        if (info->info1543) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1543);
        break;
    case 1544:
        if (info->info1544) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1544);
        break;
    case 1545:
        if (info->info1545) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1545);
        break;
    case 1546:
        if (info->info1546) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1546);
        break;
    case 1547:
        if (info->info1547) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1547);
        break;
    case 1548:
        if (info->info1548) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1548);
        break;
    case 1549:
        if (info->info1549) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1549);
        break;
    case 1550:
        if (info->info1550) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1550);
        break;
    case 1552:
        if (info->info1552) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1552);
        break;
    case 1553:
        if (info->info1553) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1553);
        break;
    case 1554:
        if (info->info1554) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1554);
        break;
    case 1555:
        if (info->info1555) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1555);
        break;
    case 1556:
        if (info->info1556) {
            break;
        }

        NFSSVC_SAFE_FREE(info->info1556);
        break;
    }
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
