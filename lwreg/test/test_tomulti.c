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
 * Copyright (C) BeyondTrust Software. All rights reserved.
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
