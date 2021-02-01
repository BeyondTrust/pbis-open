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
 *       eventlog_utils.c
 *
 * Abstract:
 *
 *        Event utilities
 */
#include "includes.h"


DWORD
UmnEvtGetFQDN(
    char *pszFQDNHostname, 
    size_t len) 
{
    DWORD dwError = 0;
    char pszHostname[1024];

    const struct addrinfo hints = {
        .ai_flags = AI_CANONNAME | AI_ADDRCONFIG,
        .ai_family = AF_UNSPEC,
        .ai_socktype = 0,
        .ai_protocol = 0,
        .ai_canonname = NULL,
        .ai_addr = NULL,
        .ai_next = NULL
    };

    struct addrinfo *paddrs= NULL;
    struct addrinfo *paddr = NULL;

    dwError = gethostname(pszHostname, sizeof(pszHostname)); 
    if (dwError) 
    {
        goto error;
    }

    dwError = getaddrinfo(pszHostname, NULL, &hints, &paddrs);
    if (dwError) 
    {
        UMN_LOG_ERROR("Failed obtaining fully qualified domain name getaddrinfo: %s\n", gai_strerror(dwError));
        goto error;
    }

    for(paddr = paddrs; paddr != NULL; paddr = paddrs->ai_next) {
        if (paddr->ai_canonname) {
            strncpy(pszFQDNHostname, paddr->ai_canonname, len - 1);
            if (len > 0 ) {
                pszFQDNHostname[len - 1] = '\0';
            }

            if (strlen(pszFQDNHostname) < strlen(paddr->ai_canonname)) {
                dwError = ENAMETOOLONG;
            }
            
            break;
        }
    }

cleanup:
    if (paddrs) {
        freeaddrinfo(paddrs);
    }

    return dwError;

error:
    goto cleanup;
}


DWORD 
UmnEvtSetEventComputerName(
    const char *pszEventComputerName) 
{
    DWORD dwError = LW_ERROR_SUCCESS;
    char pszComputerName[1024] = {0};

    strncpy(pszComputerName, pszEventComputerName, sizeof(pszComputerName) - 1);
    pszComputerName[sizeof(pszComputerName) - 1] = '\0';

    BOOLEAN isTruncated = (strlen(pszComputerName) < strlen(pszEventComputerName));
    dwError = LwMbsToWc16s(pszComputerName, &gEventComputerName);

    return (dwError 
            ? dwError 
            : ((isTruncated) 
                ? LW_ERROR_ERRNO_ENAMETOOLONG  
                : LW_ERROR_SUCCESS));
}

PCWSTR 
UmnEvtGetEventComputerName() 
{
    return gEventComputerName;
}

void 
UmnEvtFreeEventComputerName() 
{
    LwRtlWC16StringFree(&gEventComputerName);
}
