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


const unsigned char schannel_sig_sign[] = { 0x77, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00 };
const unsigned char schannel_sig_seal[] = { 0x77, 0x00, 0x7a, 0x00, 0xff, 0xff, 0x00, 0x00 };


void schn_sign_digest(unsigned char sess_key[16],
                      const unsigned char nonce[8],
                      const unsigned char schannel_sig[8],
                      const struct schn_blob *blob,
                      unsigned char digest[16])
{
    unsigned char init_buffer[4];
    unsigned char dig[16];
    MD5_CTX ctx;
    HMAC_CTX hmac_ctx;
    unsigned int digest_len;

    memset(init_buffer, 0, sizeof(init_buffer));
    memset(&ctx, 0, sizeof(ctx));
    memset(&hmac_ctx, 0, sizeof(hmac_ctx));

    MD5_Init(&ctx);
    MD5_Update(&ctx, init_buffer, 4);
    MD5_Update(&ctx, schannel_sig, 8);
    if (memcmp((void*)schannel_sig, (void*)schannel_sig_seal, 8) == 0)
    {
        MD5_Update(&ctx, nonce, 8);
    }
    MD5_Update(&ctx, blob->base, blob->len);
    MD5_Final(dig, &ctx);

    HMAC_Init(&hmac_ctx, (void*)sess_key, 16, EVP_md5());
    HMAC_Update(&hmac_ctx, (unsigned char*)dig, sizeof(dig));
    HMAC_Final(&hmac_ctx, (unsigned char*)digest, &digest_len);

    HMAC_CTX_cleanup(&hmac_ctx);
}


void schn_sign_get_seq_number(void *sec_ctx,
                              uint32 sender_flags,
                              uint8 seq_number[8])
{
    struct schn_auth_ctx *schn_ctx = NULL;

    schn_ctx = (struct schn_auth_ctx*)sec_ctx;

    /* LSB last */
    seq_number[0] = (uint8)((schn_ctx->seq_num >> 24) & 0xff);
    seq_number[1] = (uint8)((schn_ctx->seq_num >> 16) & 0xff);
    seq_number[2] = (uint8)((schn_ctx->seq_num >> 8)  & 0xff);
    seq_number[3] = (uint8)((schn_ctx->seq_num)       & 0xff);

    /* LSB first */
    seq_number[4] = (uint8)((sender_flags)       & 0xff);
    seq_number[5] = (uint8)((sender_flags >> 8)  & 0xff);
    seq_number[6] = (uint8)((sender_flags >> 16) & 0xff);
    seq_number[7] = (uint8)((sender_flags >> 24) & 0xff);
}


void schn_sign_update_seqnum(const unsigned char digest[8],
                             const unsigned char sess_key[16],
                             uint32 *seq_num,
                             unsigned char sequence[8])
{
    unsigned char init_buffer[4];
    unsigned char dig[16];
    unsigned char seq_key[16];
    HMAC_CTX hmac_ctx;
    unsigned int dig_len, seq_key_len;
    RC4_KEY rc4_key;

    memset(init_buffer, 0, sizeof(init_buffer));
    memset(&hmac_ctx, 0, sizeof(hmac_ctx));
    memset(&rc4_key, 0, sizeof(rc4_key));

    HMAC_Init(&hmac_ctx, (unsigned char*)sess_key, 16, EVP_md5());
    HMAC_Update(&hmac_ctx, (unsigned char*)init_buffer, 4);
    HMAC_Final(&hmac_ctx, (unsigned char*)dig, &dig_len);
    HMAC_CTX_cleanup(&hmac_ctx);

    HMAC_Init(&hmac_ctx, (unsigned char*)dig, 16, EVP_md5());
    HMAC_Update(&hmac_ctx, (unsigned char*)digest, 8);
    HMAC_Final(&hmac_ctx, (unsigned char*)seq_key, &seq_key_len);
    HMAC_CTX_cleanup(&hmac_ctx);

    RC4_set_key(&rc4_key, sizeof(seq_key), (unsigned char*)seq_key);
    RC4(&rc4_key, 8, (unsigned char*)sequence,(unsigned char*)sequence);

    (*seq_num)++;
}


void schn_seal_generate_key(const unsigned char sess_key[16],
                            const unsigned char seq_number[8],
                            unsigned char seal_key[16])
{
    unsigned char init_buffer[4] = {0, 0, 0, 0};
    HMAC_CTX hmac_ctx;
    unsigned int digest_len, seal_key_len;
    unsigned char key[16];
    unsigned char digest[16];
    uint32 i;

    memset(init_buffer, 0, sizeof(init_buffer));
    memset(&hmac_ctx, 0, sizeof(hmac_ctx));

    for (i = 0; i < sizeof(key); i++)
    {
        key[i] = sess_key[i] ^ 0xf0;
    }

    HMAC_Init(&hmac_ctx, (unsigned char*)key, 16, EVP_md5());
    HMAC_Update(&hmac_ctx, (unsigned char*)init_buffer, 4);
    HMAC_Final(&hmac_ctx, digest, &digest_len);
    HMAC_CTX_cleanup(&hmac_ctx);

    memset(&hmac_ctx, 0, sizeof(hmac_ctx));

    HMAC_Init(&hmac_ctx, (unsigned char*)digest, 16, EVP_md5());
    HMAC_Update(&hmac_ctx, (unsigned char*)seq_number, 8);
    HMAC_Final(&hmac_ctx, seal_key, &seal_key_len);
    HMAC_CTX_cleanup(&hmac_ctx);
}


void schn_free_blob(struct schn_blob *b)
{
    if (b == NULL) return;

    memset(b->base, 0, b->len);
    free(b->base);

    b->base = NULL;
    b->len  = 0;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
