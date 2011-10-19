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
 *        lsa_wbc_error.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 * Authors: Gerald Carter <gcarter@likewisesoftware.com>
 *
 */

#include "wbclient.h"
#include "lsawbclient_p.h"
#include "lwnet.h"

struct _ErrorMap {
    DWORD dwError;
    wbcErr wbcError;
};

static struct _ErrorMap LsaErrorTable[] = {
    { LW_ERROR_SUCCESS, WBC_ERR_SUCCESS },
    { LW_ERROR_NOT_IMPLEMENTED, WBC_ERR_NOT_IMPLEMENTED },
    { LW_ERROR_INTERNAL, WBC_ERR_UNKNOWN_FAILURE },
    { LW_ERROR_OUT_OF_MEMORY, WBC_ERR_NO_MEMORY },
    { LW_ERROR_INVALID_SID, WBC_ERR_INVALID_SID },
    { LW_ERROR_INVALID_PARAMETER, WBC_ERR_INVALID_PARAM },
    { LW_ERROR_SERVICE_NOT_AVAILABLE, WBC_ERR_WINBIND_NOT_AVAILABLE },
    { LW_ERROR_NO_SUCH_DOMAIN, WBC_ERR_DOMAIN_NOT_FOUND },
    { LW_ERROR_INVALID_SERVICE_RESPONSE, WBC_ERR_INVALID_RESPONSE },
    { LW_ERROR_NSS_ERROR, WBC_ERR_NSS_ERROR },
    { LW_ERROR_AUTH_ERROR, WBC_ERR_AUTH_ERROR },
    { LW_ERROR_NO_SUCH_USER, WBC_ERR_UNKNOWN_USER },
    { LW_ERROR_NO_SUCH_GROUP, WBC_ERR_UNKNOWN_GROUP },
    { LW_ERROR_USER_CANNOT_CHANGE_PASSWD, WBC_ERR_PWD_CHANGE_FAILED },
};

static struct _ErrorMap WbcErrorTable[] = {
    { LW_ERROR_SUCCESS, WBC_ERR_SUCCESS },
    { LW_ERROR_NOT_IMPLEMENTED, WBC_ERR_NOT_IMPLEMENTED },
    { LW_ERROR_INTERNAL, WBC_ERR_UNKNOWN_FAILURE },
    { LW_ERROR_OUT_OF_MEMORY, WBC_ERR_NO_MEMORY },
    { LW_ERROR_INVALID_SID, WBC_ERR_INVALID_SID },
    { LW_ERROR_INVALID_PARAMETER, WBC_ERR_INVALID_PARAM },
    { LW_ERROR_SERVICE_NOT_AVAILABLE, WBC_ERR_WINBIND_NOT_AVAILABLE },
    { LW_ERROR_NO_SUCH_DOMAIN, WBC_ERR_DOMAIN_NOT_FOUND },
    { LW_ERROR_INVALID_SERVICE_RESPONSE, WBC_ERR_INVALID_RESPONSE },
    { LW_ERROR_NSS_ERROR, WBC_ERR_NSS_ERROR },
    { LW_ERROR_AUTH_ERROR, WBC_ERR_AUTH_ERROR }
};

static wbcErr map_lsa_to_wbc_error(DWORD err)
{
    int i = 0;
    size_t num_map_entries = sizeof(LsaErrorTable) / sizeof(struct _ErrorMap);

    for (i=0; i<num_map_entries; i++) {
        if (LsaErrorTable[i].dwError == err) {
            return LsaErrorTable[i].wbcError;
        }
    }

    return WBC_ERR_UNKNOWN_FAILURE;
}

wbcErr map_error_to_wbc_status(DWORD err)
{
    if (err == 0)
        return WBC_ERR_SUCCESS;

    return map_lsa_to_wbc_error(err);
}


DWORD map_wbc_to_lsa_error(wbcErr err)
{
    int i = 0;
    size_t num_map_entries = sizeof(WbcErrorTable) / sizeof(struct _ErrorMap);

    for (i=0; i<num_map_entries; i++) {
        if (WbcErrorTable[i].wbcError == err) {
            return WbcErrorTable[i].dwError;
        }
    }

    return LW_ERROR_INTERNAL;
}
