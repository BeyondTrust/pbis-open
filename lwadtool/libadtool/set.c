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
    printf ("InitAdtSetAttrAction Enter\n");
    return InitBaseAction(action);
}

DWORD ValidateAdtSetAttrAction(IN AdtActionTP action)
{
    DWORD dwError = 0;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;
    PSTR dn = NULL;

    printf ("ValidateAdtSetAttrAction Enter\n");

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
//    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;

    printf ("ExecuteAdtSetAttrAction Enter\n");

//    cleanup:
        return dwError;

//    error:
//        goto cleanup;
}

DWORD CleanUpAdtSetAttrAction(IN AdtActionTP action)
{
    printf ("CleanUpAdtSetAttrAction Enter\n");

    return CleanUpBaseAction(action);
}


