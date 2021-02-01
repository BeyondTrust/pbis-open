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

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        service-locator.c
 *
 * Abstract:
 *
 *        BeyondTrust Site Manager
 *
 *        Kerberos service locator plugin
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 *
 */
#include "config.h"
#include "lwnet-system.h"
#include "lwnet-def.h"
#include "lwnet-utils.h"
#include "lwnet.h"
#include <lwerror.h>
#include <krb5/krb5.h>
#include <krb5/locate_plugin.h>
#include <lwkrb5.h>

static
DWORD
LocatorEaiToLwError(
    int eai
    );

static
krb5_error_code
LocatorInit(
    krb5_context context,
    PVOID *ppvUserData
    );

static
VOID
LocatorFini(
    PVOID pvUserData
    );

static
krb5_error_code
LocatorLookup(
    PVOID pvUserData,
    enum locate_service_type svc,
    const char *pszRealm,
    int iSocktype,
    int iFamily,
    int (*pfAddCallback)(void *pvAddCallbackData,int socktype,struct sockaddr *),
    PVOID pvAddCallbackData
    );

krb5plugin_service_locate_ftable service_locator = {
    0, //minor_version
    LocatorInit,
    LocatorFini,
    LocatorLookup
};


static
DWORD
LocatorEaiToLwError(
    int eai
    )
{
    switch (eai)
    {
        case 0:
            return ERROR_SUCCESS;
        case EAI_SERVICE:
            return LW_ERROR_KRB5_EAI_SERVICE;
        case EAI_NONAME:
#ifdef EAI_NODATA
        case EAI_NODATA:
            return LW_ERROR_KRB5_EAI_NODATA;
#endif
        case EAI_MEMORY:
            return ERROR_OUTOFMEMORY;
        default:
            return LW_ERROR_KRB5_EAI_FAIL;
    }
}

static
krb5_error_code
LocatorInit(
    krb5_context context,
    PVOID *ppvUserData
    )
{
    *ppvUserData = context;
    return 0;
}

static
VOID
LocatorFini(
    PVOID pvUserData
    )
{
}

static
krb5_error_code
LocatorLookup(
    PVOID pvUserData,
    enum locate_service_type svc,
    const char *pszRealm,
    int iSocktype,
    int iFamily,
    int (*pfAddCallback)(void *pvAddCallbackData,int socktype,struct sockaddr *),
    PVOID pvAddCallbackData
    )
{
    PLWNET_DC_INFO pDCInfo = NULL;
    DWORD dwError = 0;
    struct addrinfo hints = { 0 };
    struct addrinfo* pAddrInfo = NULL;
    krb5_error_code kError = 0;
    DWORD dwFlags = DS_KDC_REQUIRED;

    if (svc != locate_service_master_kdc && svc != locate_service_kdc)
    {
        dwError = KRB5_PLUGIN_NO_HANDLE;
        BAIL_ON_LWNET_ERROR(dwError); 
    }

    dwError = LWNetGetDCName(
                NULL,
                pszRealm,
                NULL,
                dwFlags,
                &pDCInfo
                );
    BAIL_ON_LWNET_ERROR(dwError); 

    hints.ai_family = iFamily;
    hints.ai_socktype = iSocktype;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_NUMERICHOST;

    dwError = LocatorEaiToLwError(getaddrinfo(
                pDCInfo->pszDomainControllerAddress,
                "88",
                &hints,
                &pAddrInfo));
    BAIL_ON_LWNET_ERROR(dwError); 

    kError = pfAddCallback(
                pvAddCallbackData,
                pAddrInfo->ai_socktype,
                pAddrInfo->ai_addr);
    if (kError)
    {
        dwError = LwTranslateKrb5Error(
                (krb5_context)pvAddCallbackData,
                kError,
                __FUNCTION__,
                __FILE__,
                __LINE__);
        goto error;
    }

cleanup:
    if (pAddrInfo)
    {
        freeaddrinfo(pAddrInfo);
    }
    if (pDCInfo)
    {
        LWNetFreeDCInfo(pDCInfo);
    }
    switch(dwError)
    {
        case 0:
            return ERROR_SUCCESS;
        case DNS_ERROR_BAD_PACKET:
            return KRB5_PLUGIN_NO_HANDLE;
        default:
            return KRB5KRB_ERR_GENERIC;
    }

error:
    goto cleanup;
}
