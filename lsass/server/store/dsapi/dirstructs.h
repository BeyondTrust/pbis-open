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
 *        dirstructs.h
 *
 * Abstract:
 *
 *
 *      BeyondTrust Directory Wrapper Interface
 *
 *      Structure definitions
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#ifndef __DIRSTRUCTS_H__
#define __DIRSTRUCTS_H__

typedef struct _DIRECTORY_PROVIDER_INFO
{
    DirectoryType dirType;
    PSTR          pszProviderPath;

} DIRECTORY_PROVIDER_INFO, *PDIRECTORY_PROVIDER_INFO;

typedef struct _DIRECTORY_PROVIDER
{
    LONG refCount;

    PSTR                   pszProviderName;
    PVOID                  pLibHandle;
    PFNSHUTDOWNDIRPROVIDER pfnShutdown;
    PDIRECTORY_PROVIDER_FUNCTION_TABLE pProviderFnTbl;

    PDIRECTORY_PROVIDER_INFO pProviderInfo;

} DIRECTORY_PROVIDER, *PDIRECTORY_PROVIDER;

typedef struct _DIRECTORY_CONTEXT
{
    HANDLE              hBindHandle;
    PDIRECTORY_PROVIDER pProvider;

} DIRECTORY_CONTEXT, *PDIRECTORY_CONTEXT;

typedef struct _DIRECTORY_GLOBALS
{
    pthread_mutex_t mutex;

    PDIRECTORY_PROVIDER pProvider;

} DIRECTORY_GLOBALS, *PDIRECTORY_GLOBALS;

#endif /* __DIRSTRUCTS_H__ */
