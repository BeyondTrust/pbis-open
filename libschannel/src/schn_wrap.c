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
 * Wrap the packet prior to sending
 */

uint32 schn_wrap(void                 *sec_ctx,
                 uint32                sec_level,
                 struct schn_blob     *in,
                 struct schn_blob     *out,
                 struct schn_tail     *tail)
{
    uint32 status = schn_s_ok;
    struct schn_auth_ctx *schn_ctx = NULL;
    unsigned char *schannel_sig = NULL;
    unsigned char sess_key[16], nonce[8], seq_number[8], digest[16];
    uint32 sender_flags;
    unsigned char seal_key[16];

    schn_ctx = (struct schn_auth_ctx*)sec_ctx;

    memset(sess_key, 0, sizeof(digest));
    memset(nonce, 0, sizeof(nonce));
    memset(seq_number, 0, sizeof(seq_number));
    memset(digest, 0, sizeof(digest));

    out->len  = in->len;
    out->base = malloc(out->len ? out->len : 1);
    if (out->base == NULL) {
        status = schn_s_no_memory;
        goto error;
    }

    memcpy(out->base, in->base, out->len);

    /* Nonce ("pseudo_bytes" call is to be replaced with "bytes"
       once we're ready to properly reseed the generator) */
    RAND_pseudo_bytes((unsigned char*)nonce, sizeof(nonce));

    memcpy(sess_key, schn_ctx->session_key, 16);

    /* Select proper schannel signature */
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

    /* Digest */
    schn_sign_digest(sess_key, nonce, schannel_sig, out, digest);

    sender_flags = schn_ctx->sender_flags;
    schn_sign_get_seq_number(schn_ctx, sender_flags, seq_number);

    if (sec_level == SCHANNEL_SEC_LEVEL_PRIVACY) {
        RC4_KEY key_nonce, key_data;

        memset(&key_nonce, 0, sizeof(key_nonce));
        memset(&key_data, 0, sizeof(key_data));

        /* Prepare sealing key */
        schn_seal_generate_key(schn_ctx->session_key, seq_number, seal_key);

        /* Encrypt the key */
        RC4_set_key(&key_nonce, sizeof(seal_key), (unsigned char*)seal_key);
        RC4(&key_nonce, sizeof(nonce), (unsigned char*)nonce,
            (unsigned char*)nonce);

        /* Encrypt the payload */
        RC4_set_key(&key_data, sizeof(seal_key), (unsigned char*)seal_key);
        RC4(&key_data, out->len, (unsigned char*)out->base,
            (unsigned char*)out->base);
    }

    /* Sequence number */
    schn_sign_update_seqnum(digest, sess_key, &schn_ctx->seq_num, seq_number);

    memcpy(tail->signature,  schannel_sig, 8);
    memcpy(tail->digest,     digest,       8);
    memcpy(tail->seq_number, seq_number,   8);
    memcpy(tail->nonce,      nonce,        8);

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
