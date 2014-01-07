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


/*
 * Unwrap the packet after receiving
 */

uint32 schn_unwrap(void                 *sec_ctx,
                   uint32                sec_level,
                   struct schn_blob     *in,
                   struct schn_blob     *out,
                   struct schn_tail     *tail)
{
    uint32 status = schn_s_ok;
    struct schn_auth_ctx *schn_ctx = NULL;
    uint8 seq_number[8], digest[8];
    uint32 sender_flags;
    unsigned char *schannel_sig;
    uint8 seal_key[16];

    schn_ctx = (struct schn_auth_ctx*)sec_ctx;

    switch (sec_level) {
    case SCHANNEL_SEC_LEVEL_INTEGRITY:
        schannel_sig = (unsigned char*)schannel_sig_sign;
        break;

    case SCHANNEL_SEC_LEVEL_PRIVACY:
        schannel_sig = (unsigned char*)schannel_sig_seal;
        break;

    default:
        status = schn_s_unsupported_protect_level;
        goto error;
    }

    out->len  = in->len;
    out->base = malloc(out->len ? out->len : 1);
    if (out->base == NULL) {
        status = schn_s_no_memory;
        goto error;
    }

    memcpy(out->base, in->base, out->len);

    /* if we're an initiator we should expect a packet from acceptor */
    sender_flags = (schn_ctx->sender_flags == SCHANNEL_INITIATOR_FLAGS) ?
                    SCHANNEL_ACCEPTOR_FLAGS : SCHANNEL_INITIATOR_FLAGS;

    /* create original sequence number that should be used
       to sign the received packet */
    schn_sign_get_seq_number(schn_ctx, sender_flags, seq_number);

    /* decode received sequence number and compare the result
       with expected sequence number */
    schn_sign_update_seqnum(tail->digest,
                            schn_ctx->session_key,
                            &schn_ctx->seq_num,
                            tail->seq_number);

    if (memcmp((void*)tail->seq_number,
               (void*)seq_number,
               sizeof(tail->seq_number))) {
        status = schn_s_invalid_credentials;
        goto error;
    }

    /* check whether schannel signature is correct */
    if (memcmp((void*)tail->signature,
               (void*)schannel_sig,
               sizeof(tail->signature))) {
        status = schn_s_invalid_credentials;
        goto error;
    }

    if (sec_level == SCHANNEL_SEC_LEVEL_PRIVACY) {
        RC4_KEY key_nonce, key_data;

        memset(&key_nonce, 0, sizeof(key_nonce));
        memset(&key_data, 0, sizeof(key_data));

        /* Prepare sealing key */
        schn_seal_generate_key(schn_ctx->session_key,
                               tail->seq_number, seal_key);

        /* Decrypt nonce */
        RC4_set_key(&key_nonce, sizeof(seal_key), (unsigned char*)seal_key);
        RC4(&key_nonce, sizeof(tail->nonce), (unsigned char*)tail->nonce,
            (unsigned char*)tail->nonce);

        /* Decrypt the payload */
        RC4_set_key(&key_data, sizeof(seal_key), (unsigned char*)seal_key);
        RC4(&key_data, out->len, (unsigned char*)out->base,
            (unsigned char*)out->base);

    }

    /* check the packet payload digest */
    schn_sign_digest(schn_ctx->session_key,
                     tail->nonce, schannel_sig,
                     out, digest);

    if (memcmp((void*)tail->digest,
               (void*)digest,
               sizeof(tail->digest))) {
        status = schn_s_invalid_credentials;
        goto error;
    }

cleanup:
    return status;

error:
    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
