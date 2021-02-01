/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
