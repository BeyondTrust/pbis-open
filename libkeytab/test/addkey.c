/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#include <stdlib.h>
#include <ktsystem.h>
#include <keytab.h>


int main(const int argc, const char *argv[])
{
    const PSTR ktfile = "/etc/krb5.keytab";

    DWORD dwError = 0;
    PSTR pszPrincipal = NULL;
    PSTR pszKey = NULL;
    PSTR pszDcName = NULL;
    DWORD dwKeyLen;

    if (argc < 3) {
        printf("Usage: addkey <principal> <password> <dcname>\n");
        return -1;
    }

    pszPrincipal = strdup(argv[1]);
    if (pszPrincipal == NULL) return -1;

    pszKey = strdup(argv[2]);
    if (pszKey == NULL) return -1;

    pszDcName = strdup(argv[3]);
    if (pszDcName == NULL) return -1;

    dwKeyLen = strlen(pszKey);


    dwError = KtKrb5AddKey(pszPrincipal, (PVOID)pszKey, dwKeyLen,
                           NULL, ktfile, pszDcName, -1);

    free(pszPrincipal);
    free(pszKey);
    free(pszDcName);

    return dwError;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
