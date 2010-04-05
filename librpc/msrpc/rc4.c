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

#include "rc4.h"


#define SWAP(a,b) { unsigned char temp; temp = (a); (a) = (b); (b) = temp; }

void rc4init(struct rc4ctx *ctx, unsigned char *key, size_t keylen)
{
    int i;
    unsigned char j = 0;

    for (i = 0; i < sizeof(ctx->S); i++) {
	ctx->S[i] = i;
    }
    
    for (i = 0; i < sizeof(ctx->S); i++) {
	j +=  (ctx->S[i] + key[i % keylen]);
        // ctx->S[j] does not access S out of bounds because S has size 256,
        // and j is an unsigned char (can only have values 0-255).
	SWAP(ctx->S[i], ctx->S[j]);
    }
}


void rc4crypt(struct rc4ctx *ctx, unsigned char *data, size_t len)
{
    unsigned int i;

    ctx->i = 0;
    ctx->j = 0;

    for (i = 0; i < len; i++) {
	unsigned char s;

	ctx->i++;
	ctx->j += ctx->S[ctx->i];

	SWAP(ctx->S[ctx->i], ctx->S[ctx->j]);

	s = ctx->S[ctx->i] + ctx->S[ctx->j];
	data[i] = data[i] ^ ctx->S[s];
    }
}


void rc4(unsigned char *data, size_t dlen, unsigned char *key, size_t klen)
{
    struct rc4ctx ctx;
    rc4init(&ctx, key, klen);
    rc4crypt(&ctx, data, dlen);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
