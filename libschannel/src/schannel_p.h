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

#ifndef _SCHANNEL_P_H_
#define _SCHANNEL_P_H_


void schn_sign_digest(unsigned char sess_key[16],
                      const unsigned char nonce[8],
                      const unsigned char schannel_sig[8],
                      const struct schn_blob *blob,
                      unsigned char digest[16]);

void schn_sign_get_seq_number(void *sec_ctx,
                              uint32 sender_flags,
                              uint8 seq_number[8]);

void schn_sign_update_seqnum(const unsigned char digest[8],
                             const unsigned char sess_key[16],
                             uint32 *seq_num,
                             unsigned char sequence[8]);

void schn_seal_generate_key(const unsigned char sess_key[16],
                            const unsigned char seq_number[8],
                            unsigned char seal_key[16]);

/*
  Status codes taken from dcerpc/include/dce/rpcsts.idl
*/
#define schn_s_no_memory                 (0x16c9a012)  /* rpc_s_no_memory */
#define schn_s_unsupported_protect_level (0x16c9a0e0)  /* rpc_s_unsupported_protect_level */
#define schn_s_invalid_credentials       (0x16c9a0e2)  /* rpc_s_invalid_credentials */
#define schn_s_ok                        (0x00000000)  /* rpc_s_ok */


#endif /* _SCHANNEL_P_H_ */
