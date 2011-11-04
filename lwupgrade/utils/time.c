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

#include "includes.h"

DWORD
UpParseDateString(
    PCSTR  pszTimeInterval,
    PDWORD pdwTimeInterval
    )
{
    DWORD  dwError = 0;
    DWORD  dwTimeInterval = 0;
    PSTR   pszTimeIntervalLocal = 0;
    DWORD  dwTimeIntervalLocalLen = 0;
    DWORD  dwUnitMultiplier = 0;
    PSTR   pszUnitCode = NULL;
    
    LwStripWhitespace(pszTimeIntervalLocal, TRUE, TRUE);

    BAIL_ON_INVALID_STRING(pszTimeInterval);
        
    dwError = LwAllocateString(
                    pszTimeInterval, 
                    &pszTimeIntervalLocal
                    );
    BAIL_ON_UP_ERROR(dwError);

    dwTimeIntervalLocalLen = strlen(pszTimeIntervalLocal);
    
    pszUnitCode = pszTimeIntervalLocal + dwTimeIntervalLocalLen - 1;

    if (isdigit((int)(*pszUnitCode))) 
    {
        dwUnitMultiplier = 1;
    }

    else 
    {

        switch(*pszUnitCode) 
        {
            case 's':
            case 'S':
    	        dwUnitMultiplier = 1;
    	        break;
	        
            case 'm':
            case 'M':
    	        dwUnitMultiplier = UP_SECONDS_IN_MINUTE;
    	        break;

            case 'h':
            case 'H':
                dwUnitMultiplier = UP_SECONDS_IN_HOUR;
                break;

            case 'd':
            case 'D':
                dwUnitMultiplier = UP_SECONDS_IN_DAY;
                break;

            default:
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_UP_ERROR(dwError);
                break;
        }

        *pszUnitCode = ' ';
    }
    
    LwStripWhitespace(pszTimeIntervalLocal, TRUE, TRUE);
    
    dwTimeInterval = (DWORD) atoi(pszTimeIntervalLocal) * dwUnitMultiplier;
    
    *pdwTimeInterval = dwTimeInterval;
    
cleanup:
    
    LW_SAFE_FREE_STRING(pszTimeIntervalLocal);
    
    return dwError;

error:
    
    goto cleanup;
}

