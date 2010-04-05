/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#include <string.h>
#include <lwrpc/types.h>
#include <md5.h>
#include <hmac_md5.h>


void hmac_md5_init(hmac_md5_ctx *ctx, unsigned char *key, size_t keylen)
{
    unsigned char hmac_key[16];
    size_t len = 0;
    int i;

    if (keylen > 64) {
        md5(hmac_key, key, keylen);
        len = sizeof(hmac_key);

    } else {
        memcpy(hmac_key, key, keylen);
        len = keylen;
    }

    for (i = 0; i < 64; i++) {
       ctx->ipad[i] = (i < len) ? hmac_key[i] : 0x0;
       ctx->ipad[i] ^= 0x36;
       ctx->opad[i] = (i < len) ? hmac_key[i] : 0x0;
       ctx->opad[i] ^= 0x5c;
    }

    md5init(&ctx->ctx);
    md5update(&ctx->ctx, ctx->ipad, 64);
}


void hmac_md5_update(hmac_md5_ctx *ctx, unsigned char *msg, size_t msglen)
{
    md5update(&ctx->ctx, msg, msglen);
}


void hmac_md5_final(hmac_md5_ctx *ctx, unsigned char digest[16])
{
    struct md5context outer_ctx;

    md5final(&ctx->ctx, digest);

    md5init(&outer_ctx);
    md5update(&outer_ctx, ctx->opad, 64);
    md5update(&outer_ctx, digest, 16);
    md5final(&outer_ctx, digest);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
