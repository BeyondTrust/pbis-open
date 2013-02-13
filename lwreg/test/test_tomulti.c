/*
 * Copyright Likewise Software    2004-2009
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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        test_tomulti.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Registry .REG parser test harness
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Adam Bernstein (abernstein@likewise.com)
 */
#include <parse/includes.h>
#include <shellutil/rsutils.h>


int main(int argc, char *argv[])
{
    PIV_CONVERT_CTX ivHandle = NULL;
    PCHAR strDefaultList[] = {
        "1: Now is the time",
        "2: For all good people",
        "3: All work and no play",
        "4: Makes Jack a dull boy",
        NULL,
    };

    PCHAR *argcList = NULL;
    PCHAR *multiStringList = strDefaultList;
    PBYTE multiString = NULL;
    ssize_t multiStringLen = 0;
    PSTR exportString = NULL;
    DWORD exportStringLen = 0;
    DWORD dwError = 0;
    DWORD count = 0;


    if (argc > 1)
    {
        dwError = RegAllocateMemory(sizeof(PCHAR) * argc, (PVOID*)&argcList);
        BAIL_ON_REG_ERROR(dwError);

        for (count=1; count < argc; count++)
        {
            argcList[count-1] = argv[count];
        }
        multiStringList = argcList;
    }

    RegIconvConvertOpen(&ivHandle, "ucs-2le", "utf-8");
    RegMultiStrsToByteArray(multiStringList,
                                &multiString,
                                &multiStringLen);
    RegExportEntry("HKLM_LINUX/likewise/registry/devel",
                   NULL,
                   0,
                   NULL,
                   REG_KEY,
                   NULL,
                   0,
                   &exportString,
                   &exportStringLen);
    printf("%s\n", exportString);

    RegExportEntry(NULL,
                   NULL,
                   0,
                   "testkey1",
                   REG_MULTI_SZ,
                   multiString,
                   multiStringLen,
                   &exportString,
                   &exportStringLen);
    printf("%s\n", exportString);

    RegIconvConvertClose(ivHandle);

cleanup:
    return dwError;

error:
    goto cleanup;

}
