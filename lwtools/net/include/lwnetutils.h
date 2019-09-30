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
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwutils.h
 *
 * Abstract:
 *
 *        BeyondTrust system net utilities internal util functions
 *
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Wei Fu (wfu@likewise.com)
 */

#ifndef __LW_NETUTILS_H__
#define __LW_NETUTILS_H__

#define BAIL_ON_LTNET_ERROR(dwError) \
	if (dwError) goto error;


#define LTNET_SAFE_FREE_STRING(str) \
        do {                      \
           if (str) {             \
              LwNetFreeString(str); \
              (str) = NULL;       \
           }                      \
        } while(0);

#define LTNET_SAFE_CLEAR_FREE_STRING(str)       \
        do {                                  \
           if (str) {                         \
              if (*str) {                     \
                 memset(str, 0, strlen(str)); \
              }                               \
              LwNetFreeString(str);             \
              (str) = NULL;                   \
           }                                  \
        } while(0);

#define LTNET_SAFE_FREE_MEMORY(mem) \
        do {                      \
           if (mem) {             \
              LwNetFreeMemory(mem); \
              (mem) = NULL;       \
           }                      \
        } while(0);

#define LTNET_SAFE_FREE_STRING_ARRAY(ppszArray)               \
        do {                                                \
           if (ppszArray) {                                 \
               LwNetFreeNullTerminatedStringArray(ppszArray); \
               (ppszArray) = NULL;                          \
           }                                                \
        } while (0);


#define LTNET_IS_NULL_OR_EMPTY_STR(str) (!(str) || !(*(str)))


DWORD
LwNetAllocateMemory(
    size_t Size,
    LW_PVOID* ppMemory
    );

DWORD
LwNetReallocMemory(
    LW_PVOID pMemory,
    LW_PVOID* ppNewMemory,
    size_t Size
    );

VOID
LwNetFreeMemory(
    PVOID pMemory
    );

DWORD
LwNetAllocateString(
    PCSTR  pszInputString,
    PSTR* ppszOutputString
    );

VOID
LwNetFreeString(
    PSTR pszString
    );

VOID
LwNetFreeStringArray(
    DWORD dwCount,
    PSTR * ppStringArray
    );

VOID
LwNetFreeNullTerminatedStringArray(
    PSTR * ppStringArray
    );

DWORD
LwNetWC16StringAllocateFromCString(
    OUT PWSTR* ppszNewString,
    IN PCSTR pszOriginalString
    );

VOID
LwNetWC16StringFree(
    PWSTR pwszString
    );

VOID
LwNetFreeWC16StringArray(
    DWORD dwCount,
    PWSTR* ppwszArray
    );

DWORD
LwNetAllocateSidFromCString(
    OUT PSID* Sid,
    IN PCSTR StringSid
    );

BOOLEAN
LwNetCheckUnsignedInteger(
    PCSTR pszIntegerCandidate
    );

DWORD
LwNetAppendStringArray(
    PDWORD pdwCount,
    PWSTR** pppwszArray,
    PWSTR pwszString
    );

#endif /* __LW_NETUTILS_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
