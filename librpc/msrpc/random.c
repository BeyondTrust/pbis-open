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

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>

#include <md5.h>


#define RANDOMDEV  "/dev/urandom"

void get_random_buffer(unsigned char *out, size_t outlen)
{
    int fd, i;
    ssize_t count = 0;
    char hostname[512];
    struct timeval tv;
    unsigned char hash[16];
    int len = 0;
    
    if (out == NULL || outlen < 0) return;

    fd = open(RANDOMDEV, O_RDONLY);
    if (fd >= 0) {
        count = read(fd, out, outlen);
        close(fd);
    }

    if (fd < 0 || count < outlen) {
        /* if number of bytes read was too small or simply
           an error occured while reading urandom file
           use timestamp and hostname both hashed with md5
           to simulate random bytes  */

        if (gethostname(hostname, sizeof(hostname)) == 0) {
            /* ensure the returned name is no longer than size of the buffer */
            hostname[sizeof(hostname) - 1] = '\0';
            len = strlen(hostname);
            md5(hash, (unsigned char*)hostname, len);
        }

        if (gettimeofday(&tv, NULL) == 0) {
            md5(hash, (unsigned char*)&tv, sizeof(tv));
        }

        /* fill the output buffer with 16 bytes chunks, each being
           a hash of preceding one */
        i = 0;
        do {
            if (i >= sizeof(hash)) {
                int j, off;
                unsigned char in[16];

                off = ((i/sizeof(hash))-1)*sizeof(hash);
                for (j = 0; j < sizeof(hash) && (off + j) < len ; j++) {
                    in[j] = out[off + j];
                }
                md5(hash, in, sizeof(in));
            }

            memcpy((void*)&(out[i]), (void*)hash,
                   ((outlen-i) < sizeof(hash)) ?
                   outlen % sizeof(hash) : sizeof(hash));

            i += sizeof(hash);
        } while (i < outlen);
    }
}


static const char randomchar[] = "abcdefghijklmnoprstuvwxyz"
                                 "ABCDEFGHIJKLMNOPRSTUVWXYZ"
                                 "-+/*,.;:!<=>%'&()0123456789";

void get_random_string(char *out, size_t outsize)
{
    int i = 0;

    if (out == NULL || outsize == 0) return;

    get_random_buffer((unsigned char*)out, outsize - 1);
    if (out[0] == '\0') return;

    /* replace each byte with its ASCII equivalent */
    for (i = 0; i < (outsize - 1); i++) {
        out[i] = randomchar[out[i] % (sizeof(randomchar)-1)];
    }

    /* terminate the string */
    out[outsize - 1] = '\0';
}


void get_random_string_w16(wchar16_t *out, size_t outsize)
{
    int i = 0;
    unsigned char *bytes = NULL;

    if (out == NULL || outsize == 0) return;

    bytes = (unsigned char*) malloc(sizeof(unsigned char) * outsize);
    if (bytes == NULL) {
        out[0] = '\0';
        return;
    }

    get_random_buffer((unsigned char*)bytes, outsize - 1);
    if (bytes[0] == '\0') goto done;

    /* replace each byte with its alphanum equivalent
       converted to 2-byte unicode */
    for (i = 0; i < (outsize - 1); i++) {
        out[i] = (wchar16_t)randomchar[bytes[i] % (sizeof(randomchar)-1)];
    }

    /* terminate the string */
    out[outsize - 1] = '\0';

done:
    memset(bytes, 0, sizeof(sizeof(unsigned char) * outsize));
    free(bytes);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
