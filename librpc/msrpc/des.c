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


static const
UINT8 pc1[56] = { 57,   49,   41,   33,   25,   17,    9,
		   1,   58,   50,   42,   34,   26,   18,
		  10,    2,   59,   51,   43,   35,   27,
		  19,   11,    3,   60,   52,   44,   36,
		  63,   55,   47,   39,   31,   23,   15,
		   7,   62,   54,   46,   38,   30,   22,
		  14,    6,   61,   53,   45,   37,   29,
		  21,   13,    5,   28,   20,   12,    4 };

static const
int shifts[16] = { 1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1 };

static const
UINT8 pc2[48] = { 14,   17,   11,   24,    1,    5,
		   3,   28,   15,    6,   21,   10,
		  23,   19,   12,    4,   26,    8,
		  16,    7,   27,   20,   13,    2,
		  41,   52,   31,   37,   47,   55,
		  30,   40,   51,   45,   33,   48,
		  44,   49,   39,   56,   34,   53,
		  46,   42,   50,   36,   29,   32 };

static const
UINT8 ip[64] = { 58,   50,   42,   34,   26,   18,   10,   2,
	         60,   52,   44,   36,   28,   20,   12,   4,
	         62,   54,   46,   38,   30,   22,   14,   6,
	         64,   56,   48,   40,   32,   24,   16,   8,
	         57,   49,   41,   33,   25,   17,    9,   1,
	         59,   51,   43,   35,   27,   19,   11,   3,
	         61,   53,   45,   37,   29,   21,   13,   5,
	         63,   55,   47,   39,   31,   23,   15,   7 };

static const
UINT8 e[48] = { 32,    1,    2,    3,    4,    5,
		 4,    5,    6,    7,    8,    9,
		 8,    9,   10,   11,   12,   13,
		12,   13,   14,   15,   16,   17,
		16,   17,   18,   19,   20,   21,
		20,   21,   22,   23,   24,   25,
		24,   25,   26,   27,   28,   29,
		28,   29,   30,   31,   32,    1 };

static const
UINT8 s[8][4][16] = {
    {
	{ 14,  4,  13,  1,   2, 15,  11,  8,   3, 10,   6, 12,   5,  9,   0,  7 },
	{ 0, 15,   7,  4,  14,  2,  13,  1,  10,  6,  12, 11,   9,  5,   3,  8 },
	{ 4,  1,  14,  8,  13,  6,   2, 11,  15, 12,   9,  7,   3, 10,   5,  0},
	{ 15, 12,   8,  2,   4,  9,   1,  7,   5, 11,   3, 14,  10,  0,   6, 13}
    },
    {
	{ 15,  1,   8, 14,   6, 11,   3,  4,   9,  7,   2, 13,  12,  0,   5, 10 },
	{ 3, 13,   4,  7,  15,  2,   8, 14,  12,  0,   1, 10,   6,  9,  11,  5},
	{ 0, 14,   7, 11,  10,  4,  13,  1,   5,  8,  12,  6,   9,  3,   2, 15 },
	{ 13,  8,  10,  1,   3, 15,   4,  2,  11,  6,   7, 12,   0,  5,  14,  9 }
    },
    {
	{ 10,  0,   9, 14,   6,  3,  15,  5,   1, 13,  12,  7,  11,  4,   2,  8 },
	{ 13,  7,   0,  9,   3,  4,   6, 10,   2,  8,   5, 14,  12, 11,  15,  1},
	{ 13,  6,   4,  9,   8, 15,   3,  0,  11,  1,   2, 12,   5, 10,  14,  7},
	{ 1, 10,  13,  0,   6,  9,   8,  7,   4, 15,  14,  3,  11,  5,   2, 12 }
    },
    {
	{ 7, 13,  14,  3,   0,  6,   9, 10,   1,  2,   8,  5,  11, 12,   4, 15 },
	{ 13,  8,  11,  5,   6, 15,   0,  3,   4,  7,   2, 12,   1, 10,  14,  9 },
	{ 10,  6,   9,  0,  12, 11,   7, 13,  15,  1,   3, 14,   5,  2,   8,  4 },
	{ 3, 15,   0,  6,  10,  1,  13,  8,   9,  4,   5, 11,  12,  7,   2, 14 }
    },
    {
	{ 2, 12,   4,  1,   7, 10,  11,  6,   8,  5,   3, 15,  13,  0,  14,  9 },
	{ 14, 11,   2, 12,   4,  7,  13,  1,   5,  0,  15, 10,   3,  9,   8,  6 },
	{ 4,  2,   1, 11,  10, 13,   7,  8,  15,  9,  12,  5,   6,  3,   0, 14 },
	{ 11,  8,  12,  7,   1, 14,   2, 13,   6, 15,   0,  9,  10,  4,   5,  3 }
    },
    {
	{ 12,  1,  10, 15,   9,  2,   6,  8,   0, 13,   3,  4,  14,  7,   5, 11 },
	{ 10, 15,   4,  2,   7, 12,   9,  5,   6,  1,  13, 14,   0, 11,   3,  8 },
	{ 9, 14,  15,  5,   2,  8,  12,  3,   7,  0,   4, 10,   1, 13,  11,  6 },
	{ 4,  3,   2, 12,   9,  5,  15, 10,  11, 14,   1,  7,   6,  0,   8, 13 }
    },
    {
	{ 4, 11,   2, 14,  15,  0,   8, 13,   3, 12,   9,  7,   5, 10,   6,  1 },
	{ 13,  0,  11,  7,   4,  9,   1, 10,  14,  3,   5, 12,   2, 15,   8,  6 },
	{ 1,  4,  11, 13,  12,  3,   7, 14,  10, 15,   6,  8,   0,  5,   9,  2 },
	{ 6, 11,  13,  8,   1,  4,  10,  7,   9,  5,   0, 15,  14,  2,   3, 12 }
    },
    {
	{ 13,  2,   8,  4,   6, 15,  11,  1,  10,  9,   3, 14,   5,  0,  12,  7 },
	{ 1, 15,  13,  8,  10,  3,   7,  4,  12,  5,   6, 11,   0, 14,   9,  2 },
	{ 7, 11,   4,  1,   9, 12,  14,  2,   0,  6,  10, 13,  15,  3,   5,  8 },
	{ 2,  1,  14,  7,   4, 10,   8, 13,  15, 12,   9,  0,   3,  5,   6, 11 }
    }
};


static const
UINT8 p[32] = { 16,    7,    20,    21,    29,    12,    28,    17,
	         1,   15,    23,    26,     5,    18,    31,    10,
		 2,    8,    24,    14,    32,    27,     3,     9,
	        19,   13,    30,     6,    22,    11,     4,    25 };

static const
UINT8 fp[64] = { 40,   8,    48,    16,    56,    24,    64,   32,
		 39,   7,    47,    15,    55,    23,    63,   31,
		 38,   6,    46,    14,    54,    22,    62,   30,
		 37,   5,    45,    13,    53,    21,    61,   29,
		 36,   4,    44,    12,    52,    20,    60,   28,
		 35,   3,    43,    11,    51,    19,    59,   27,
		 34,   2,    42,    10,    50,    18,    58,   26,
		 33,   1,    41,     9,    49,    17,    57,   25 };


static void bitblock(UINT8 *out, size_t outlen, const UINT8 *in)
{
    int i, count, bytecount;

    count = 0, bytecount = 0;

    while (count < outlen) {
	UINT8 b = in[bytecount++];

	for (i = 0; i < 8 && count < outlen; i++) {
	    out[count++] = (b & (1 << (7 - i))) ? 1 : 0;
	}
    }
}


static void revbitblock(UINT8 *out, size_t outlen, const UINT8 *in,
			size_t inlen)
{
    int i, j = 0;
    UINT8 b;

    while (j < outlen) {
	b = 0;

	for (i = 0; i < 8 && i < inlen; i++) {
	    b = b*2 + ((in[i + 8*j]) ? 1 : 0);
	}

	out[j++] = b;
    }
}


static void permute(UINT8 *pk, const UINT8 *k, const UINT8 *pt, size_t size)
{
    int i;
    for (i = 0; i < size; i++) pk[i] = k[pt[i]-1];
}


static void leftshift(UINT8 *out, const UINT8 *in, int shift, size_t size)
{
    int i, j = 0;

    /* copy shifted bytes to the out buffer */
    for (i = 0; i + shift < size && j < size; i++) {
	out[j++] = in[i + shift];
    }

    /* cycle first shifted bytes to the end */
    for (i = 0; i < shift && j < size; i++) {
	out[j++] = in[i];
    }
}


static void xor(UINT8 *out, const UINT8 *in1, const UINT8 *in2, size_t size)
{
    int i;
    for (i = 0; i < size; i++) out[i] = in1[i] ^ in2[i];
}


static UINT8* ef(UINT8 out[32], const UINT8 in[32], const UINT8 key[48])
{
    const int size = 48;
    const int sbox_count = 8;
    int i, snum;
    UINT8 si, sj;
    UINT8 expanded[48] = {0};
    UINT8 sboxed[32] = {0};

    /* expand 32 bits into 48 bits using e selection table */
    for (i = 0; i < size; i++) expanded[i] = in[e[i]-1];

    /* xor the expanded 48 bits with the key */
    xor(expanded, expanded, key, size);

    /* process 8 sboxes (48 bits) with sbox functions */
    for (snum = 0; snum < sbox_count; snum++) {
	UINT8 sbox_in[6];
	UINT8 v[8] = {0};
	
	/* prepare sbox input (6 bits) */
	memcpy(sbox_in, &expanded[snum*6], sizeof(sbox_in));
	si = sbox_in[0]*2 + sbox_in[5];
	revbitblock(&sj, 1, &sbox_in[1], 4);

	/* get the sbox function value and turn in back into bitblock */
	bitblock(v, 8, &(s[snum][si][sj]));

	/* we need only the lowest 4 bits */
	memcpy(&sboxed[snum*4], &v[4], 4);
    }

    /* finally permute sboxed 32 bits */
    permute(out, sboxed, p, sizeof(p));

    return out;
}


void str_to_key(UINT8 outkey[8], const UINT8 inkey[7])
{
    int i;

    outkey[0] = inkey[0] >> 1;
    outkey[1] = ((inkey[0]&0x01) << 6) | (inkey[1] >> 2);
    outkey[2] = ((inkey[1]&0x03) << 5) | (inkey[2] >> 3);
    outkey[3] = ((inkey[2]&0x07) << 4) | (inkey[3] >> 4);
    outkey[4] = ((inkey[3]&0x0F) << 3) | (inkey[4] >> 5);
    outkey[5] = ((inkey[4]&0x1F) << 2) | (inkey[5] >> 6);
    outkey[6] = ((inkey[5]&0x3F) << 1) | (inkey[6] >> 7);
    outkey[7] = inkey[6]&0x7F;

    for (i = 0; i < 8; i++) outkey[i] = (outkey[i] << 1);
}


int des56(UINT8 *out, const UINT8 *in, size_t len, const UINT8 key[7])
{
    UINT8 *buf, *outbuf;
    size_t padlen;
    int i, j;
    UINT8 deskey[8];
    UINT8 k[64], pk[56];
    UINT8 c[17][28], d[17][28];
    UINT8 pkn[16][48];
    UINT8 m[64], pm[64], om[64];
    UINT8 l[17][32], r[17][32];

    padlen = (len % 8) > 0 ? 8 - (len % 8) : 0;
    buf = (UINT8*) malloc((len + padlen) * sizeof(UINT8));
    if (buf == NULL) return 0;
    outbuf = (UINT8*) malloc((len + padlen) * sizeof(UINT8));
    if (outbuf == NULL) return 0;

    memset(buf, 0, len + padlen);
    memcpy(buf, in, len);
    memset(outbuf, 0, len + padlen);

    /* prepare 8-byte des key with parity bits */
    str_to_key(deskey, key);

    /* process 64-bit (8-byte) blocks */
    for (i = 0; i < (len + padlen); i += 8) {
	UINT8 preout[64];

	/* prepare key block */
	bitblock(k, sizeof(k), deskey);

	permute(pk, k, pc1, sizeof(pk));

	memcpy(c[0], pk, 28);
	memcpy(d[0], &pk[28], 28);

	for (j = 1; j <= 16; j++) {
	    leftshift(c[j], c[j-1], shifts[j-1], sizeof(c[j]));
	    leftshift(d[j], d[j-1], shifts[j-1], sizeof(d[j]));
	}

	for (j = 1; j <= 16; j++) {
	    UINT8 cd[56];
	    
	    memcpy(cd, c[j], 28);
	    memcpy(&cd[28], d[j], 28);
	    permute(pkn[j-1], cd, pc2, 48);
	}

	/* prepare message block */
	bitblock(m, sizeof(m), &buf[i]);

	permute(pm, m, ip, sizeof(pm));

	/* prepare left- and right-side 32-bit blocks */
	memcpy(l[0], pm, sizeof(l[0]));
	memcpy(r[0], &pm[32], sizeof(r[0]));

	for (j = 1; j <= 16; j++) {
	    UINT8 efret[32] = {0};

	    /* l[j] = r[j-1] */
	    memcpy(l[j], r[j-1], sizeof(l[j]));

	    /* r[j] = l[j-1] + ef(r[j-1], pkn[j-1]) */
	    ef(efret, r[j-1], pkn[j-1]);
	    xor(r[j], l[j-1], efret, sizeof(r[j]));
	}

	/* concatenate left and right 16 bits in reversed order */
	memcpy(preout, r[16], sizeof(r[16]));
	memcpy(&preout[32], l[16], sizeof(l[16]));

	/* do the final permutation of concatenated bits */
	permute(om, preout, fp, sizeof(om));

	/* turn the bitblock back into bytes */
	revbitblock(&outbuf[i], len - i, om, 8);
    }

    /* return the result */
    memcpy(out, outbuf, len);

    free(buf);
    free(outbuf);

    return i;
}


void des128(UINT8 out[8], const UINT8 in[8], const UINT8 key[16])
{
    UINT8 inout[8];

    des56(inout, in, sizeof(inout), key);
    des56(out, inout, sizeof(inout), &key[8]);
}


void des112(UINT8 out[8], const UINT8 in[8], const UINT8 key[14])
{
    UINT8 inout[8];

    des56(inout, in, sizeof(inout), key);
    des56(out, inout, sizeof(inout), &key[7]);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
