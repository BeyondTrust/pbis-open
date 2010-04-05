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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <lwrpc/types.h>
#include <wc16str.h>
#include <random.h>
#include <md4.h>
#include <des.h>


void EncodePassBufferW16(unsigned char buffer[516], const wchar16_t *pass)
{
    wchar16_t newpass[512];
    size_t pass_size;

    /* Force string into little-endian byte ordering */
    wc16stowc16les(newpass, pass, sizeof(newpass) / sizeof(*newpass));

    pass_size = wc16slen(pass) * sizeof(wchar16_t);

    memcpy((void*)&buffer[512 - pass_size], (void*)newpass, pass_size);

    /* Set initial random bytes to enhance security */
    get_random_buffer((unsigned char*)buffer, 512 - pass_size);
    
    /* set the password length - the last 4 bytes */
    buffer[512] = (unsigned char)((pass_size) & 0xff);
    buffer[513] = (unsigned char)((pass_size >> 8) & 0xff);
    buffer[514] = (unsigned char)((pass_size >> 16) & 0xff);
    buffer[515] = (unsigned char)((pass_size >> 24) & 0xff);
}


void EncodePassBuffer(unsigned char buffer[516], const char* pass)
{
    unsigned char newpass[512];
    size_t newpass_len;

    newpass_len = strlen(pass) * sizeof(wchar16_t);
    mbstowc16s((wchar16_t*)newpass, pass, newpass_len);

    memcpy((void*)&buffer[512 - newpass_len], (void*)newpass, newpass_len);

    /* Set initial random bytes to enhance security */
    get_random_buffer((unsigned char*)buffer, 512 - newpass_len);

    /* set the password length - the last 4 bytes */
    buffer[512] = (unsigned char)((newpass_len) & 0xff);
    buffer[513] = (unsigned char)((newpass_len >> 8) & 0xff);
    buffer[514] = (unsigned char)((newpass_len >> 16) & 0xff);
    buffer[515] = (unsigned char)((newpass_len >> 24) & 0xff);

    memset(newpass, 0, sizeof(newpass));
}


void md4hash(UINT8 h[16], const wchar16_t *password)
{
    size_t size = 0;
    size_t len = 0;
    wchar16_t *password_le = NULL;

    memset(h, 0, sizeof(h));

    len = wc16slen(password);
    size = len * sizeof(wchar16_t);
    password_le = malloc(size + sizeof(wchar16_t));
    if (password_le == NULL) return;

    /* Force string into little-endian byte ordering */
    wc16stowc16les(password_le, password, len);

    md4(h, (UINT8*)password_le, size);

    free(password_le);
}


void deshash(UINT8 h[16], const wchar16_t *password)
{
    const size_t max_passlen = 14;
    const UINT8 input[] = "KGS!@#$%";
    const size_t input_len = 8;

    size_t len;
    unsigned char *mbspass;
    int i;

    /* Clear the hash first */
    memset(h, 0, sizeof(h));

    /* password can be 14 characters long at most */
    len = wc16slen(password);
    if (len > 14) return;

    mbspass = (unsigned char*) malloc(max_passlen);
    if (mbspass == NULL) return;

    memset((void*)mbspass, 0, max_passlen);
    wc16stombs((char*)mbspass, password, len + 1);
    for (i = 0; i < len; i++) mbspass[i] = toupper(mbspass[i]);

    des56(h, input, input_len, mbspass);
    des56(&h[8], input, input_len, &mbspass[7]);

    free(mbspass);
}


void encrypt_challenge(UINT8 out[24], UINT8 chal[8], UINT8 key[16])
{
    UINT8 k[21];

    memset(k, 0, sizeof(k));
    memcpy((void*)k, (void*)key, 16);

    des56(&out[0],  chal, 8, &k[0]);
    des56(&out[8],  chal, 8, &k[7]);
    des56(&out[16], chal, 8, &k[14]);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
