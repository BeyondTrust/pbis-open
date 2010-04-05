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

#include <stdlib.h>
#include <string.h>

#include <lwrpc/types.h>
#include <md4.h>

static const
UINT8 padding[64] = {
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


static void md4transform(struct md4context *ctx);

static void enc(unsigned char *out, unsigned int *in, size_t len)
{
    int i, j;
    for (i = 0, j = 0; j < len; i++, j+=4) {
	out[j]   = (unsigned char)(in[i] & 0xff);
	out[j+1] = (unsigned char)((in[i] >> 8)  & 0xff);
	out[j+2] = (unsigned char)((in[i] >> 16) & 0xff);
	out[j+3] = (unsigned char)((in[i] >> 24) & 0xff);
    }
}


static void dec(unsigned int *out, unsigned char *in, size_t len)
{
    int i, j;
    
    for (i = 0, j = 0; j < len; i++, j+=4) {
	out[i] = (unsigned int)in[j] |
	         ((unsigned int)in[j+1]) << 8 |
	         ((unsigned int)in[j+2]) << 16 |
	         ((unsigned int)in[j+3]) << 24;
    }
}


void md4init(struct md4context *ctx)
{
    if (!ctx) return;

    ctx->A = 0x67452301;
    ctx->B = 0xefcdab89;
    ctx->C = 0x98badcfe;
    ctx->D = 0x10325476;

    ctx->count[0] = 0;
    ctx->count[1] = 0;

    memset(ctx->block, 0, sizeof(ctx->block));
}


void md4update(struct md4context *ctx, const unsigned char *buf,
	       size_t len)
{
    int i, index, block_len;

    index = (int)((ctx->count[0] >> 3) & 0x3f);
    ctx->count[0] += len * 8;
    if (ctx->count[0] < len * 8) ctx->count[1]++;
    ctx->count[1] += len >> (32-8);

    block_len = 64 - index;
    
    if (len >= block_len) {
	memcpy(&ctx->block[index], buf, block_len);
	md4transform(ctx);

	for (i = block_len; i + 63 < len; i+=64) {
	    memcpy(ctx->block, &buf[i], sizeof(ctx->block));
	    md4transform(ctx);
	}

	index = 0;

    } else {
	i = 0;
    }

    memcpy(&ctx->block[index], &buf[i], len - i);
}


static void md4transform(struct md4context *ctx)
{
    unsigned int A, B, C, D;
    int i, j;
    unsigned int X[16] = {0};

    for (i = 0, j = 0; i < sizeof(X) / sizeof(X[0]); i++, j+=4) {
	dec(&X[i], &ctx->block[j], 4);
    }

    A = ctx->A;
    B = ctx->B;
    C = ctx->C;
    D = ctx->D;

#define LROT(v, n)         (((v) << (n)) | ((v) >> (32-(n))))

    /* Round 1 */
#define F(x, y, z)         (((x)&(y)) | ((~x)&(z)))
#define Fx(a,b,c,d, k,s)   ((a) = LROT(((a) + F((b),(c),(d)) + X[k]), s))
    
    Fx(A,B,C,D,  0, 3);
    Fx(D,A,B,C,  1, 7);
    Fx(C,D,A,B,  2,11);
    Fx(B,C,D,A,  3,19);
    Fx(A,B,C,D,  4, 3);
    Fx(D,A,B,C,  5, 7);
    Fx(C,D,A,B,  6,11);
    Fx(B,C,D,A,  7,19);
    Fx(A,B,C,D,  8, 3);
    Fx(D,A,B,C,  9, 7);
    Fx(C,D,A,B, 10,11);
    Fx(B,C,D,A, 11,19);
    Fx(A,B,C,D, 12, 3);
    Fx(D,A,B,C, 13, 7);
    Fx(C,D,A,B, 14,11);
    Fx(B,C,D,A, 15,19);

    /* Round 2 */
#define G(x, y, z)         (((x)&(y)) | ((x)&(z)) | ((y)&(z)))
#define Gx(a,b,c,d, k,s)   ((a) = LROT(((a) + G((b),(c),(d)) + X[k] + 0x5A827999), s))

    Gx(A,B,C,D,  0, 3);
    Gx(D,A,B,C,  4, 5);
    Gx(C,D,A,B,  8, 9);
    Gx(B,C,D,A, 12,13);
    Gx(A,B,C,D,  1, 3);
    Gx(D,A,B,C,  5, 5); 
    Gx(C,D,A,B,  9, 9);
    Gx(B,C,D,A, 13,13);
    Gx(A,B,C,D,  2, 3);
    Gx(D,A,B,C,  6, 5);
    Gx(C,D,A,B, 10, 9);
    Gx(B,C,D,A, 14,13);
    Gx(A,B,C,D,  3, 3);
    Gx(D,A,B,C,  7, 5);
    Gx(C,D,A,B, 11, 9);
    Gx(B,C,D,A, 15,13);

    /* Round 3 */
#define H(x, y, z)         ((x)^(y)^(z))
#define Hx(a,b,c,d, k,s)   ((a) = LROT(((a) + H((b),(c),(d)) + X[k] + 0x6ED9EBA1), s))

    Hx(A,B,C,D,  0, 3);
    Hx(D,A,B,C,  8, 9);
    Hx(C,D,A,B,  4,11);
    Hx(B,C,D,A, 12,15);
    Hx(A,B,C,D,  2, 3);
    Hx(D,A,B,C, 10, 9); 
    Hx(C,D,A,B,  6,11);
    Hx(B,C,D,A, 14,15);
    Hx(A,B,C,D,  1, 3); 
    Hx(D,A,B,C,  9, 9); 
    Hx(C,D,A,B,  5,11);
    Hx(B,C,D,A, 13,15);
    Hx(A,B,C,D,  3, 3);
    Hx(D,A,B,C, 11, 9);
    Hx(C,D,A,B,  7,11);
    Hx(B,C,D,A, 15,15);

    ctx->A += A;
    ctx->B += B;
    ctx->C += C;
    ctx->D += D;

    memset(X, 0, sizeof(X));
}


void md4final(struct md4context *ctx, unsigned char digest[16])
{
    unsigned char b[8];
    unsigned int index, padding_len;

    enc(b, ctx->count, sizeof(b));

    index = (unsigned int)((ctx->count[0] >> 3) & 0x3f);
    padding_len = (index < 56) ? (56 - index) : (120 - index);
    md4update(ctx, padding, padding_len);

    md4update(ctx, b, sizeof(b));

    enc(&digest[0],  &ctx->A, 4);
    enc(&digest[4],  &ctx->B, 4);
    enc(&digest[8],  &ctx->C, 4);
    enc(&digest[12], &ctx->D, 4);

    memset(ctx, 0, sizeof(*ctx));
}


void md4(unsigned char out[16], const unsigned char *in, size_t len)
{
    struct md4context ctx;

    md4init(&ctx);
    md4update(&ctx, in, len);
    md4final(&ctx, out);

    memset(&ctx, 0, sizeof(struct md4context));
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
