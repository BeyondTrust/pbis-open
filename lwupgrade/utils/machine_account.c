/*
 * Copyright (c) Likewise Software.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 * Abstract:
 *
 * Authors:
 * 
 */
#include "includes.h"

#define SCHANNEL_WKSTA     2
#define SCHANNEL_DOMAIN    4
#define SCHANNEL_BDC       6

LSA_MACHINE_ACCOUNT_FLAGS
UpConvertSchannelTypeToMachineAccountFlags(
    IN DWORD SchannelType
    )
{
    LSA_MACHINE_ACCOUNT_FLAGS accountFlags = 0;

    switch (SchannelType)
    {
        case SCHANNEL_WKSTA:
            accountFlags = LSA_MACHINE_ACCOUNT_TYPE_WORKSTATION;
            break;
        case SCHANNEL_DOMAIN:
            accountFlags = LSA_MACHINE_ACCOUNT_TYPE_DC;
            break;
        case SCHANNEL_BDC:
            accountFlags = LSA_MACHINE_ACCOUNT_TYPE_BDC;
            break;
        default:
            // Default to workstation
            accountFlags = LSA_MACHINE_ACCOUNT_TYPE_WORKSTATION;
            break;
    }

    return accountFlags;
}

DWORD
UpConvertTimeUnixToWindows(
    IN time_t UnixTime,
    OUT PLONG64 pWindowsTime
    )
{
    DWORD dwError = 0;
    LONG64 windowsTime = 0;

#if SIZEOF_TIME_T > 4
    if ((LONG64) UnixTime < - 11644473600LL)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_UP_ERROR(dwError);
    }
#endif

    windowsTime = (LONG64) UnixTime + 11644473600LL;
    if (windowsTime < 0)
    {
        dwError = ERROR_ARITHMETIC_OVERFLOW;
        BAIL_ON_UP_ERROR(dwError);
    }
    windowsTime *= 10000000LL;
    if (windowsTime < 0)
    {
        dwError = ERROR_ARITHMETIC_OVERFLOW;
        BAIL_ON_UP_ERROR(dwError);
    }

error:
    if (dwError)
    {
        windowsTime = 0;
    }

    *pWindowsTime = windowsTime;

    return 0;
}
