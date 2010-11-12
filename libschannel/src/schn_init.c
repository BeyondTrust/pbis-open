/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
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
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include "includes.h"


uint32 schn_init_creds(struct schn_auth_ctx   *ctx,
                       struct schn_blob       *creds)
{
    const uint32 flag1 = LW_HTOL32(0x00000000);
    const uint32 flag2 = LW_HTOL32(0x00000017);

    size_t domain_name_len = 0;
    size_t fqdn_len = 0;
    size_t machine_name_len = 0;
    char *b = NULL;
    int i;

    fqdn_len = strlen((char*) ctx->fqdn);
    domain_name_len  = strlen((char*)ctx->domain_name);
    machine_name_len = strlen((char*)ctx->machine_name);

    b = (char*)creds->base;

    i = 0;
    memcpy(&b[i], &flag1, sizeof(uint32));
    i += sizeof(uint32);
    memcpy(&b[i], &flag2, sizeof(uint32));
    i += sizeof(uint32);
    strncpy((char*)&b[i], (char*)ctx->domain_name, domain_name_len + 1);
    i += domain_name_len + 1;
    strncpy((char*)&b[i], (char*)ctx->machine_name, machine_name_len + 1);
    i += machine_name_len + 1;
    b[i++] = fqdn_len;
    strncpy((char*)&b[i], (char*)ctx->fqdn, fqdn_len + 1);
    i += fqdn_len + 1;
    b[i++] = machine_name_len;
    strncpy((char*)&b[i], (char*)ctx->machine_name, machine_name_len + 1);
    i += machine_name_len + 1;

    creds->len = i;

    return 0;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
