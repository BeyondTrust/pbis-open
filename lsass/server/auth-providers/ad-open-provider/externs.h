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
 *        externs.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Active Directory Authentication Provider
 *
 *        External Variables
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __EXTERNS_H__
#define __EXTERNS_H__

extern pthread_rwlock_t gADGlobalDataLock;

extern pthread_mutex_t gADDefaultDomainLock;

#define ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock)       \
        if (!bInLock) {                                    \
           pthread_rwlock_rdlock(&gADGlobalDataLock);      \
           bInLock = TRUE;                                 \
        }

#define LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock)       \
        if (bInLock) {                                     \
           pthread_rwlock_unlock(&gADGlobalDataLock);      \
           bInLock = FALSE;                                \
        }

#define ENTER_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock)       \
        if (!bInLock) {                                    \
           pthread_rwlock_wrlock(&gADGlobalDataLock);      \
           bInLock = TRUE;                                 \
        }

#define LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock)       \
        if (bInLock) {                                     \
           pthread_rwlock_unlock(&gADGlobalDataLock);      \
           bInLock = FALSE;                                \
        }

extern PCSTR gpszADProviderName;

extern LSA_PROVIDER_FUNCTION_TABLE gADProviderAPITable;

// please put all new globals in the LSA_AD_PROVIDER_STATE
// structures which are stored in the following list:
extern LSA_LIST_LINKS gLsaAdProviderStateList;

extern PADCACHE_PROVIDER_FUNCTION_TABLE gpCacheProvider;

extern BOOLEAN gbMultiTenancyEnabled;

#endif /* __EXTERNS_H__ */

