/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
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
 * license@likewisesoftware.com
 */



/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        dirstructs.h
 *
 * Abstract:
 *
 *
 *      Likewise Directory Wrapper Interface
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
