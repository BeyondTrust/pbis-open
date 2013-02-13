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
 *        lsa_wbc_guid.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 *
 */

#include "wbclient.h"
#include "lsawbclient_p.h"
#include "util_str.h"
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

void
wbcUuidToWbcGuid(
    IN const uuid_t uuid,
    OUT struct wbcGuid *pGuid
    )
{
    // Use big endian. This matches what libuuid uses.
    pGuid->time_low =   (uuid[0] << 24) |
                        (uuid[1] << 16) |
                        (uuid[2] << 8) |
                        (uuid[3] << 0);

    pGuid->time_mid =   (uuid[4] << 8) |
                        (uuid[5] << 0);

    pGuid->time_hi_and_version =    (uuid[6] << 8) |
                                    (uuid[7] << 0);

    memcpy(pGuid->clock_seq, &uuid[8], 2);
    memcpy(pGuid->node, &uuid[10], 6);
}

static
void
WbcGuidToUuid(
    IN const struct wbcGuid *pGuid,
    OUT uuid_t uuid
    )
{
    // Use big endian. This matches what libuuid uses.
    uuid[0] = (unsigned char)(pGuid->time_low >> 24);
    uuid[1] = (unsigned char)(pGuid->time_low >> 16);
    uuid[2] = (unsigned char)(pGuid->time_low >> 8);
    uuid[3] = (unsigned char)(pGuid->time_low >> 0);

    uuid[4] = (unsigned char)(pGuid->time_mid >> 8);
    uuid[5] = (unsigned char)(pGuid->time_mid >> 0);

    uuid[6] = (unsigned char)(pGuid->time_hi_and_version >> 8);
    uuid[7] = (unsigned char)(pGuid->time_hi_and_version >> 0);

    memcpy(&uuid[8], &pGuid->clock_seq, 2);
    memcpy(&uuid[10], &pGuid->node, 6);
}

wbcErr
wbcGuidToString(
    const struct wbcGuid *guid,
    char **guid_string
    )
{
    DWORD error = 0;
    uuid_t uuid = { 0 };
    PSTR pGuidString = NULL;

    WbcGuidToUuid(
        guid,
        uuid);

    pGuidString = _wbc_malloc_zero(37, NULL);
    BAIL_ON_NULL_PTR(pGuidString, error);

    uuid_unparse(
            uuid,
            pGuidString);

    *guid_string = pGuidString;

cleanup:
    if (error)
    {
        _WBC_FREE(pGuidString);
        *guid_string = NULL;
    }
    return map_error_to_wbc_status(error);
}

wbcErr
wbcStringToGuid(
    const char *guid_string,
    struct wbcGuid *guid
    )
{
    DWORD error = 0;
    uuid_t uuid = { 0 };

    memset(guid, 0, sizeof(*guid));

    if (uuid_parse((PSTR)guid_string, uuid) < 0)
    {
        error = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERR(error);
    }

    wbcUuidToWbcGuid(
        uuid,
        guid);

cleanup:
    return error;
}
