/*
 * Copyright BeyondTrust Software
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
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU LESSER GENERAL
 * PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR
 * WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY
 * BEYONDTRUST, PLEASE CONTACT BEYONDTRUST AT beyondtrust.com/contact
 */
#include <stdio.h>
#include <unistd.h>
#include "lw/types.h"
#include "lw/winerror.h"
#include "lw/base.h"
#include "lwerror.h"
#include "lwmem.h"
#include "lwstr.h"

const int MIN_ARG_COUNT = 2;

static void usage(const char *arg0)
{
    printf("usage: %s <hex-string>\n", arg0);
    printf("Displays relative security descriptor validation error information.\n");
    printf("This accepts hex string representations of security descriptors as shown in the gpagent log,\n");
    printf("and performs the same verification as gpagent does.\n");
}

int main(const int argc, const char * const argv[])
{
    DWORD error = ERROR_SUCCESS;
    const char *hex_rep;

    UCHAR *buffer_ptr = NULL;
    DWORD buffer_len= -1;

    UINT error_subcode = 0;

    if (geteuid() != 0) {
        fprintf(stderr, "This program requires super-user privileges.\n");
        return LW_ERROR_ACCESS_DENIED;
    }

    if (argc < MIN_ARG_COUNT) {
        usage(argv[0]);
        return 1;
    }

    hex_rep = argv[1];
    error = LwHexStrToByteArray(hex_rep, NULL, &buffer_ptr, &buffer_len);

    if (error) {
        fprintf(stderr, "Failed converting input to byte array; error code: %lu.\nThe supplied input may be corrupted or incomplete.\n", (unsigned long)error);
    } else {
        PSECURITY_DESCRIPTOR_RELATIVE rel_security_descriptor = (PSECURITY_DESCRIPTOR_RELATIVE) (PBYTE) buffer_ptr;

        if (!RtlValidRelativeSecurityDescriptorWithErrorSubcode(rel_security_descriptor, buffer_len, DACL_SECURITY_INFORMATION, &error_subcode)) {
            fprintf(stderr, "Security descriptor failed validation. %s\n", RtlValidRelativeSecurityDescriptorErrorMessage(error_subcode));
        } else {
            fprintf(stderr, "Security descriptor passed validation checks.\n");
        }
    }

clean_up:
    if (buffer_ptr) {
       LwFreeMemory(buffer_ptr);
       buffer_ptr = NULL;
    }

    return 0;
}
