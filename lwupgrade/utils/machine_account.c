/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
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
