/*
 * Copyright (c) BeyondTrust Software.  All rights Reserved.
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
 * BEYONDTRUST SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH BEYONDTRUST SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY BEYONDTRUST SOFTWARE, PLEASE CONTACT BEYONDTRUST SOFTWARE AT
 * license@beyondtrust.com
 */

/*
 * Module Name:
 *
 *        set.c
 *
 * Abstract:
 *
 *        Methods for setting attributes of directory objects.
 *
 * Authors: Author: CORP\rali
 *
 * Created on: Sep 6, 2016
 *
 */


#include "includes.h"


DWORD InitAdtSetAttrAction(IN AdtActionTP action)
{
    return InitBaseAction(action);
}

DWORD ValidateAdtSetAttrAction(IN AdtActionTP action)
{
    DWORD dwError = 0;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;
    PSTR dn = NULL;

    dwError = OpenADSearchConnectionDN(action, &(action->setAttribute.dn));
    ADT_BAIL_ON_ERROR_NP(dwError);

    SwitchToSearchConnection(action);

    if (!action->setAttribute.dn) {
        dwError = ADT_ERR_ARG_MISSING_DN;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    dwError = ProcessDash(&(action->setAttribute.dn));
    ADT_BAIL_ON_ERROR_NP(dwError);

    dwError = ResolveDN(appContext, ObjectClassAny, action->setAttribute.dn, &dn);
    ADT_BAIL_ON_ERROR_NP(dwError);
    LW_SAFE_FREE_MEMORY(action->setAttribute.dn);
    action->setAttribute.dn = dn;

    cleanup:
        return dwError;

    error:
       LW_SAFE_FREE_MEMORY(dn);
       goto cleanup;
}

DWORD ExecuteAdtSetAttrAction(IN AdtActionTP action)
{
    DWORD dwError = 0;
    DWORD  i, j = 0;
    DWORD dwMaxValues = 100;
    AttrValsT *avp = NULL;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;
    PSTR aStr = NULL;
    PSTR saveStrPtr = NULL;
    PSTR tmpStr = NULL;

    dwError = LwAllocateMemory(2 * sizeof(AttrValsT), OUT_PPVOID(&avp));
    ADT_BAIL_ON_ALLOC_FAILURE(!dwError);

    avp[0].attr = action->setAttribute.attrName;

    dwError = LwAllocateMemory(dwMaxValues * sizeof(PSTR), OUT_PPVOID(&(avp[0].vals)));
    ADT_BAIL_ON_ALLOC_FAILURE(!dwError);

    for (i = 0; i < dwMaxValues; i++)
       avp[0].vals[i] = NULL;

    if (action->setAttribute.attrValue)
    {
       dwError = LwStrDupOrNull(action->setAttribute.attrValue, &tmpStr);
       ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

       // Use semi-colon to delimit a multi-value attribute.
       aStr = strtok_r(tmpStr, ADT_LIST_DELIMITER, &saveStrPtr);
       i = 0;
       while ((aStr != NULL) && (i < dwMaxValues))
       {
           dwError = LwStrDupOrNull((PCSTR) aStr, &(avp[0].vals[i]));
           ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
           aStr = strtok_r(NULL, ADT_LIST_DELIMITER, &saveStrPtr);
           i++;
       }
    }

    dwError = ModifyADObject(appContext, action->setAttribute.dn, avp, 2);
    ADT_BAIL_ON_ERROR_NP(dwError);

    if (action->setAttribute.attrValue)
       PrintResult(appContext, LogLevelNone, "Successfully updated \"%s\" with: %s\n", 
                                  action->setAttribute.attrName,
                                  action->setAttribute.attrValue);
    else
       PrintResult(appContext, LogLevelNone, "Cleared \"%s\" attribute\n", action->setAttribute.attrName);

cleanup:

    if (avp) 
    {
        for (i = 0; avp[i].vals; ++i) 
        {
            for (j = 0; avp[i].vals[j]; ++j) 
            {
               LW_SAFE_FREE_MEMORY(avp[i].vals[j]);
            }

            LW_SAFE_FREE_MEMORY(avp[i].vals);
        }

        LW_SAFE_FREE_MEMORY(avp);
    }

    if (tmpStr)
        LW_SAFE_FREE_STRING(tmpStr);

    return dwError;

error:
    goto cleanup;
}

DWORD CleanUpAdtSetAttrAction(IN AdtActionTP action)
{
    return CleanUpBaseAction(action);
}


