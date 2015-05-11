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
