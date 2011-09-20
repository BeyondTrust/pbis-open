/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright Likewise Software
 * All rights reserved.
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
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        samba-pstore-plugin.c
 *
 * Abstract:
 *
 *        Pstore plugin implementation
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 */

#include "config.h"

#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <lw/winerror.h>
#include <lw/swab.h>

#include <lwmem.h>
#include <lwstr.h>
#include <lw/rtllog.h>
#include <lwtime.h>
#include <lwsecurityidentifier.h>

#include "samba-pstore-plugin.h"
#include <reg/regutil.h>

#include <tdb.h>

#define BAIL_ON_LSA_ERROR(error)                                      \
    if (error) {                                                      \
        LW_RTL_LOG_DEBUG("Error code %d", error); \
        goto cleanup;                                                     \
    }

struct _LSA_PSTORE_PLUGIN_CONTEXT
{
    TDB_CONTEXT* pTdb;
};

static
VOID
CleanupContext(
    IN PLSA_PSTORE_PLUGIN_CONTEXT pContext
    );

static
DWORD
SetPassword(
    IN PLSA_PSTORE_PLUGIN_CONTEXT pContext,
    IN PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo
    );

static
DWORD
DeletePassword(
    IN PLSA_PSTORE_PLUGIN_CONTEXT pContext,
    IN OPTIONAL PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo
    );

static
DWORD
TdbRead(
    TDB_CONTEXT *pTdb,
    PCSTR pKeyStart,
    PCSTR pKeyEnd,
    PVOID* ppData,
    PDWORD pDataLen
    )
{
    DWORD error = 0;
    TDB_DATA tdbKey = { 0 };
    TDB_DATA tdbData = { 0 };
    PSTR pKey = NULL;

    error = LwAllocateStringPrintf(
                    &pKey,
                    "%s/%s",
                    pKeyStart,
                    pKeyEnd);
    BAIL_ON_LSA_ERROR(error);

    tdbKey.dptr = pKey;
    tdbKey.dsize = strlen(pKey);

    tdbData = tdb_fetch(pTdb, tdbKey);
    if (!tdbData.dptr)
    {
        tdb_transaction_cancel(pTdb);
        if (tdb_error(pTdb) == TDB_ERR_NOEXIST)
        {
            error = ERROR_FILE_NOT_FOUND;
        }
        else
        {
            error = ERROR_INTERNAL_DB_ERROR;
        }
        BAIL_ON_LSA_ERROR(error);
    }

    *ppData = tdbData.dptr;
    tdbData.dptr = NULL;
    *pDataLen = tdbData.dsize;

cleanup:
    LW_SAFE_FREE_STRING(pKey);
    LW_SAFE_FREE_MEMORY(tdbData.dptr);
    return error;
}

static
DWORD
TdbStore(
    TDB_CONTEXT *pTdb,
    PCSTR pKeyStart,
    PCSTR pKeyEnd,
    PVOID pData,
    DWORD DataLen
    )
{
    DWORD error = 0;
    int ret = 0;
    TDB_DATA tdbKey = { 0 };
    TDB_DATA tdbData = { 0 };
    PSTR pKey = NULL;

    error = LwAllocateStringPrintf(
                    &pKey,
                    "%s/%s",
                    pKeyStart,
                    pKeyEnd);
    BAIL_ON_LSA_ERROR(error);

    tdbKey.dptr = pKey;
    tdbKey.dsize = strlen(pKey);

    tdbData.dptr = pData;
    tdbData.dsize = DataLen;

    if ((ret = tdb_transaction_start(pTdb)) != 0) {
        error = ERROR_INTERNAL_DB_ERROR;
        BAIL_ON_LSA_ERROR(error);
    }

    if ((ret = tdb_store(pTdb, tdbKey, tdbData, TDB_REPLACE)) != 0) {
        tdb_transaction_cancel(pTdb);
        error = ERROR_INTERNAL_DB_ERROR;
        BAIL_ON_LSA_ERROR(error);
    }

    if ((ret = tdb_transaction_commit(pTdb)) != 0) {
        error = ERROR_INTERNAL_DB_ERROR;
        BAIL_ON_LSA_ERROR(error);
    }

cleanup:
    LW_SAFE_FREE_STRING(pKey);
    return error;
}

static
DWORD
TdbDelete(
    TDB_CONTEXT *pTdb,
    PCSTR pKeyStart,
    PCSTR pKeyEnd
    )
{
    DWORD error = 0;
    int ret = 0;
    TDB_DATA tdbKey = { 0 };
    PSTR pKey = NULL;

    error = LwAllocateStringPrintf(
                    &pKey,
                    "%s/%s",
                    pKeyStart,
                    pKeyEnd);
    BAIL_ON_LSA_ERROR(error);

    tdbKey.dptr = pKey;
    tdbKey.dsize = strlen(pKey);

    if ((ret = tdb_transaction_start(pTdb)) != 0) {
        error = ERROR_INTERNAL_DB_ERROR;
        BAIL_ON_LSA_ERROR(error);
    }

    if (tdb_exists(pTdb, tdbKey))
    {
        if ((ret = tdb_delete(pTdb, tdbKey)) != 0)
        {
            tdb_transaction_cancel(pTdb);
            error = ERROR_INTERNAL_DB_ERROR;
            BAIL_ON_LSA_ERROR(error);
        }
    }

    if ((ret = tdb_transaction_commit(pTdb)) != 0) {
        error = ERROR_INTERNAL_DB_ERROR;
        BAIL_ON_LSA_ERROR(error);
    }

cleanup:
    LW_SAFE_FREE_STRING(pKey);
    return error;
}

DWORD
LsaPstorePluginInitializeContext(
    IN ULONG Version,
    IN PCSTR pName,
    OUT PLSA_PSTORE_PLUGIN_DISPATCH* ppDispatch,
    OUT PLSA_PSTORE_PLUGIN_CONTEXT* ppContext
    )
{
    DWORD error = 0;
    PSTR pSecretsPath = NULL;
    TDB_CONTEXT *pTdb = NULL;
    REG_DATA_TYPE regType = 0;
    PLSA_PSTORE_PLUGIN_CONTEXT pContext = NULL;
    static LSA_PSTORE_PLUGIN_DISPATCH dispatch = {
        .Cleanup = CleanupContext,
        .SetPasswordInfoA = SetPassword,
        .DeletePasswordInfoA = DeletePassword
    };

    if (Version != LSA_PSTORE_PLUGIN_VERSION)
    {
        error = ERROR_REVISION_MISMATCH;
        BAIL_ON_LSA_ERROR(error);
    }

    error = RegUtilGetValue(
                NULL,
                LSA_PSTORE_REG_ROOT_KEY_PATH,
                NULL,
                LSA_PSTORE_REG_ROOT_KEY_RELATIVE_PATH_PLUGINS "\\" PLUGIN_NAME,
                "SecretsPath",
                &regType,
                (PVOID*)&pSecretsPath,
                NULL);
    if (regType != REG_SZ)
    {
        error = ERROR_INVALID_DATA;
        BAIL_ON_LSA_ERROR(error);
    }
    BAIL_ON_LSA_ERROR(error);

    pTdb = tdb_open(
                    pSecretsPath,
                    0,
                    TDB_DEFAULT,
                    O_RDWR|O_CREAT,
                    0600);
    if (pTdb == NULL)
    {
        error = ERROR_INTERNAL_DB_ERROR;
        BAIL_ON_LSA_ERROR(error);
    }

    error = LwAllocateMemory(
                sizeof(*pContext),
                (PVOID*)&pContext);
    BAIL_ON_LSA_ERROR(error);

    pContext->pTdb = pTdb;
    *ppContext = pContext;

    *ppDispatch = &dispatch;

cleanup:
    LW_SAFE_FREE_STRING(pSecretsPath);
    if (error)
    {
        if (pTdb)
        {
            tdb_close(pTdb);
        }
        LW_SAFE_FREE_MEMORY(pContext);

        if (ppContext)
        {
            *ppContext = NULL;
        }
        if (ppDispatch)
        {
            *ppDispatch = NULL;
        }
    }
    return error;
}

static
VOID
CleanupContext(
    IN PLSA_PSTORE_PLUGIN_CONTEXT pContext
    )
{
    if (pContext->pTdb)
    {
        tdb_close(pContext->pTdb);
    }
    LW_SAFE_FREE_MEMORY(pContext);
}

static
DWORD
SetPassword(
    IN PLSA_PSTORE_PLUGIN_CONTEXT pContext,
    IN PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo
    )
{
    DWORD error = 0;
    PLW_SECURITY_IDENTIFIER pSid = NULL;
    DWORD schannelType = 0;
    DWORD LCT = 0;
    BYTE sambaSid[68] = { 0 };
    DWORD oldPasswordLen = 0;
    PVOID pOldPassword = NULL;

    // Move the current password to the old password
    error = TdbRead(
                    pContext->pTdb,
                    "SECRETS/MACHINE_PASSWORD",
                    pPasswordInfo->Account.NetbiosDomainName,
                    &pOldPassword,
                    &oldPasswordLen);
    if (error == ERROR_FILE_NOT_FOUND)
    {
        error = 0;
    }
    else
    {
        BAIL_ON_LSA_ERROR(error);

        if (strcmp(pOldPassword, pPasswordInfo->Password))
        {
            error = TdbStore(
                            pContext->pTdb,
                            "SECRETS/MACHINE_PASSWORD.PREV",
                            pPasswordInfo->Account.NetbiosDomainName,
                            pOldPassword,
                            oldPasswordLen);
            BAIL_ON_LSA_ERROR(error);
        }
    }

    /* Machine Password */
    // The terminating null must be stored with the password
    error = TdbStore(
                    pContext->pTdb,
                    "SECRETS/MACHINE_PASSWORD",
                    pPasswordInfo->Account.NetbiosDomainName,
                    pPasswordInfo->Password,
                    strlen(pPasswordInfo->Password) + 1);
    BAIL_ON_LSA_ERROR(error);

    /* Domain SID */

    error = LwAllocSecurityIdentifierFromString(
                    pPasswordInfo->Account.DomainSid,
                    &pSid);
    BAIL_ON_LSA_ERROR(error);

    // Samba wants the sid to be null padded and exactly 68 bytes long.
    memcpy(
            sambaSid,
            pSid->pucSidBytes,
            LW_MIN(pSid->dwByteLength, sizeof(sambaSid)));
    error = TdbStore(
                    pContext->pTdb,
                    "SECRETS/SID",
                    pPasswordInfo->Account.NetbiosDomainName,
                    sambaSid,
                    sizeof(sambaSid));
    BAIL_ON_LSA_ERROR(error);

    /* Schannel Type */

    switch(LSA_GET_MACHINE_ACCOUNT_TYPE(pPasswordInfo->Account.AccountFlags))
    {
        case LSA_MACHINE_ACCOUNT_TYPE_WORKSTATION:
            schannelType = LW_HTOL32(2);
            break;
        case LSA_MACHINE_ACCOUNT_TYPE_DC:
            schannelType = LW_HTOL32(4);
            break;
        case LSA_MACHINE_ACCOUNT_TYPE_BDC:
            schannelType = LW_HTOL32(6);
            break;
        default:
            error = ERROR_INVALID_LOGON_TYPE;
            BAIL_ON_LSA_ERROR(error);
            break;
    }

    error = TdbStore(
                    pContext->pTdb,
                    "SECRETS/MACHINE_SEC_CHANNEL_TYPE",
                    pPasswordInfo->Account.NetbiosDomainName,
                    &schannelType,
                    sizeof(DWORD));
    BAIL_ON_LSA_ERROR(error);

    /* Last Change Time */

    LCT = LW_HTOL32(LwNtTimeToWinTime(
                pPasswordInfo->Account.LastChangeTime));

    error = TdbStore(
                    pContext->pTdb,
                    "SECRETS/MACHINE_LAST_CHANGE_TIME",
                    pPasswordInfo->Account.NetbiosDomainName,
                    &LCT,
                    sizeof(DWORD));
    BAIL_ON_LSA_ERROR(error);

    LW_RTL_LOG_INFO("Wrote machine password for domain %s in secrets.tdb",
            pPasswordInfo->Account.NetbiosDomainName);

cleanup:
    if (pSid != NULL)
    {
        LwFreeSecurityIdentifier(pSid);
    }
    LW_SAFE_FREE_MEMORY(pOldPassword);
    return error;
}

static
DWORD
DeletePassword(
    IN PLSA_PSTORE_PLUGIN_CONTEXT pContext,
    IN OPTIONAL PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo
    )
{
    DWORD error = 0;

    if (!pAccountInfo || !pAccountInfo->NetbiosDomainName)
    {
        goto cleanup;
    }

    error = TdbDelete(
                    pContext->pTdb,
                    "SECRETS/MACHINE_PASSWORD",
                    pAccountInfo->NetbiosDomainName);
    BAIL_ON_LSA_ERROR(error);

    error = TdbDelete(
                    pContext->pTdb,
                    "SECRETS/MACHINE_PASSWORD.PREV",
                    pAccountInfo->NetbiosDomainName);
    BAIL_ON_LSA_ERROR(error);

    error = TdbDelete(
                    pContext->pTdb,
                    "SECRETS/SID",
                    pAccountInfo->NetbiosDomainName);
    BAIL_ON_LSA_ERROR(error);


    error = TdbDelete(
                    pContext->pTdb,
                    "SECRETS/MACHINE_SEC_CHANNEL_TYPE",
                    pAccountInfo->NetbiosDomainName);
    BAIL_ON_LSA_ERROR(error);

    error = TdbDelete(
                    pContext->pTdb,
                    "SECRETS/MACHINE_LAST_CHANGE_TIME",
                    pAccountInfo->NetbiosDomainName);
    BAIL_ON_LSA_ERROR(error);

    LW_RTL_LOG_INFO("Deleted machine password for domain %s in secrets.tdb", pAccountInfo->NetbiosDomainName);

cleanup:
    return error;
}
