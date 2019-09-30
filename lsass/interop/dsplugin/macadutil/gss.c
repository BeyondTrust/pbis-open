/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#include "../includes.h"




int
strupr(char *szDomainName);

DWORD
ADUConvertDomainToDN(
    PCSTR pszDomainName,
    PSTR* ppszDomainDN
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PSTR pszChar = NULL;
    PSTR pszDomainDN = NULL;
    CHAR  pszBuffer[256];

    memset(pszBuffer, 0, sizeof(pszBuffer));
    while ((pszChar = strchr(pszDomainName, '.'))) {
        strcat(pszBuffer,"dc=");
        strncat(pszBuffer, pszDomainName, pszChar - pszDomainName);
        strcat(pszBuffer,",");
        pszDomainName = pszChar+1;
    }
    strcat(pszBuffer, "dc=");
    strcat(pszBuffer, pszDomainName);

    dwError = LwAllocateString(pszBuffer, &pszDomainDN);
    BAIL_ON_MAC_ERROR(dwError);

    *ppszDomainDN = pszDomainDN;

    return dwError;

cleanup:

    *ppszDomainDN = NULL;
    return dwError;

error:
 
    goto cleanup;
}

int
strupr(char *szDomainName)
{
    if (!szDomainName) {
        return(0);
    }
    while (*szDomainName != '\0'){
        *szDomainName = toupper(*szDomainName);
        szDomainName++;
    }
    return (0);
}

