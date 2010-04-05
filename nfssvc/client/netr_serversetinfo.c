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

#include "includes.h"

NET_API_STATUS
NetrServerSetInfo(
    PNFSSVC_CONTEXT pContext,
    const wchar16_t *servername,
    UINT32 level,
    UINT8 *bufptr,
    UINT32 *parm_err
    )
{
    NET_API_STATUS status = ERROR_SUCCESS;
    nfssvc_NetNfsInfo info;

    BAIL_ON_INVALID_PTR(pContext, status);
    BAIL_ON_INVALID_PTR(pContext->hBinding, status);

    memset(&info, 0, sizeof(info));

    switch (level) {
    case 100:
        info.info100 = (PSERVER_INFO_100)bufptr;
        break;
    case 101:
        info.info101 = (PSERVER_INFO_101)bufptr;
        break;
    case 102:
        info.info102 = (PSERVER_INFO_102)bufptr;
        break;
    case 402:
        info.info402 = (PSERVER_INFO_402)bufptr;
        break;
    case 403:
        info.info403 = (PSERVER_INFO_403)bufptr;
        break;
    case 502:
        info.info502 = (PSERVER_INFO_502)bufptr;
        break;
    case 503:
        info.info503 = (PSERVER_INFO_503)bufptr;
        break;
    case 599:
        info.info599 = (PSERVER_INFO_599)bufptr;
        break;
    case 1005:
        info.info1005 = (PSERVER_INFO_1005)bufptr;
        break;
    case 1010:
        info.info1010 = (PSERVER_INFO_1010)bufptr;
        break;
    case 1016:
        info.info1016 = (PSERVER_INFO_1016)bufptr;
        break;
    case 1017:
        info.info1017 = (PSERVER_INFO_1017)bufptr;
        break;
    case 1018:
        info.info1018 = (PSERVER_INFO_1018)bufptr;
        break;
    case 1107:
        info.info1107 = (PSERVER_INFO_1107)bufptr;
        break;
    case 1501:
        info.info1501 = (PSERVER_INFO_1501)bufptr;
        break;
    case 1502:
        info.info1502 = (PSERVER_INFO_1502)bufptr;
        break;
    case 1503:
        info.info1503 = (PSERVER_INFO_1503)bufptr;
        break;
    case 1506:
        info.info1506 = (PSERVER_INFO_1506)bufptr;
        break;
    case 1509:
        info.info1509 = (PSERVER_INFO_1509)bufptr;
        break;
    case 1510:
        info.info1510 = (PSERVER_INFO_1510)bufptr;
        break;
    case 1511:
        info.info1511 = (PSERVER_INFO_1511)bufptr;
        break;
    case 1512:
        info.info1512 = (PSERVER_INFO_1512)bufptr;
        break;
    case 1513:
        info.info1513 = (PSERVER_INFO_1513)bufptr;
        break;
    case 1514:
        info.info1514 = (PSERVER_INFO_1514)bufptr;
        break;
    case 1515:
        info.info1515 = (PSERVER_INFO_1515)bufptr;
        break;
    case 1516:
        info.info1516 = (PSERVER_INFO_1516)bufptr;
        break;
    case 1518:
        info.info1518 = (PSERVER_INFO_1518)bufptr;
        break;
    case 1520:
        info.info1520 = (PSERVER_INFO_1520)bufptr;
        break;
    case 1521:
        info.info1521 = (PSERVER_INFO_1521)bufptr;
        break;
    case 1522:
        info.info1522 = (PSERVER_INFO_1522)bufptr;
        break;
    case 1523:
        info.info1523 = (PSERVER_INFO_1523)bufptr;
        break;
    case 1524:
        info.info1524 = (PSERVER_INFO_1524)bufptr;
        break;
    case 1525:
        info.info1525 = (PSERVER_INFO_1525)bufptr;
        break;
    case 1528:
        info.info1528 = (PSERVER_INFO_1528)bufptr;
        break;
    case 1529:
        info.info1529 = (PSERVER_INFO_1529)bufptr;
        break;
    case 1530:
        info.info1530 = (PSERVER_INFO_1530)bufptr;
        break;
    case 1533:
        info.info1533 = (PSERVER_INFO_1533)bufptr;
        break;
    case 1534:
        info.info1534 = (PSERVER_INFO_1534)bufptr;
        break;
    case 1535:
        info.info1535 = (PSERVER_INFO_1535)bufptr;
        break;
    case 1536:
        info.info1536 = (PSERVER_INFO_1536)bufptr;
        break;
    case 1537:
        info.info1537 = (PSERVER_INFO_1537)bufptr;
        break;
    case 1538:
        info.info1538 = (PSERVER_INFO_1538)bufptr;
        break;
    case 1539:
        info.info1539 = (PSERVER_INFO_1539)bufptr;
        break;
    case 1540:
        info.info1540 = (PSERVER_INFO_1540)bufptr;
        break;
    case 1541:
        info.info1541 = (PSERVER_INFO_1541)bufptr;
        break;
    case 1542:
        info.info1542 = (PSERVER_INFO_1542)bufptr;
        break;
    case 1543:
        info.info1543 = (PSERVER_INFO_1543)bufptr;
        break;
    case 1544:
        info.info1544 = (PSERVER_INFO_1544)bufptr;
        break;
    case 1545:
        info.info1545 = (PSERVER_INFO_1545)bufptr;
        break;
    case 1546:
        info.info1546 = (PSERVER_INFO_1546)bufptr;
        break;
    case 1547:
        info.info1547 = (PSERVER_INFO_1547)bufptr;
        break;
    case 1548:
        info.info1548 = (PSERVER_INFO_1548)bufptr;
        break;
    case 1549:
        info.info1549 = (PSERVER_INFO_1549)bufptr;
        break;
    case 1550:
        info.info1550 = (PSERVER_INFO_1550)bufptr;
        break;
    case 1552:
        info.info1552 = (PSERVER_INFO_1552)bufptr;
        break;
    case 1553:
        info.info1553 = (PSERVER_INFO_1553)bufptr;
        break;
    case 1554:
        info.info1554 = (PSERVER_INFO_1554)bufptr;
        break;
    case 1555:
        info.info1555 = (PSERVER_INFO_1555)bufptr;
        break;
    case 1556:
        info.info1556 = (PSERVER_INFO_1556)bufptr;
        break;
    }

    DCERPC_CALL(status,
                _NetrServerSetInfo(
                        pContext->hBinding,
                        (wchar16_t *)servername,
                        level,
                        info,
                        parm_err));

cleanup:
    return status;

error:
    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
