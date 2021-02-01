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
    uint8 seq_number[8], digest[16];
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
