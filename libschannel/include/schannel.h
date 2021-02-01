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

#ifndef _SCHANNEL_H_
#define _SCHANNEL_H_

#include <schtypes.h>

extern const unsigned char schannel_sig_sign[];
extern const unsigned char schannel_sig_seal[];

struct schn_auth_ctx {
    uint8           session_key[16];
    unsigned char  *domain_name;
    unsigned char  *fqdn;
    unsigned char  *machine_name;
    uint32          sender_flags;
    uint32          seq_num;
};


struct schn_creds {
    uint32          flags1;
    uint32          flags2;
    unsigned char  *domain_name;
    unsigned char  *machine_name;
};


struct schn_tail {
    uint8           signature[8];
    uint8           seq_number[8];
    uint8           digest[8];
    uint8           nonce[8];
};


struct schn_blob {
    void          *base;
    size_t         len;
};


#define SCHANNEL_SEC_LEVEL_INTEGRITY   (5)
#define SCHANNEL_SEC_LEVEL_PRIVACY     (6)


#define SCHANNEL_INITIATOR_FLAGS       (0x0080)
#define SCHANNEL_ACCEPTOR_FLAGS        (0x0000)


uint32 schn_init_creds(struct schn_auth_ctx *ctx,
                       struct schn_blob     *creds);

uint32 schn_wrap(void                 *sec_ctx,
                 uint32                sec_level,
                 struct schn_blob     *in,
                 struct schn_blob     *out,
                 struct schn_tail     *tail);

uint32 schn_unwrap(void                 *sec_ctx,
                   uint32                sec_level,
                   struct schn_blob     *in,
                   struct schn_blob     *out,
                   struct schn_tail     *tail);

void schn_free_blob(struct schn_blob *b);


#endif /* _SCHANNEL_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
