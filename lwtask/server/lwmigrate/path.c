/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        path.c
 *
 * Abstract:
 *
 *        BeyondTrust Task Service (LWTASK)
 *
 *        Share Migration Management
 *
 *        Path management
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

/**
 * @brief Get Local Share path based on Windows path
 *        When migrating shares, we want to create all shares under the
 *        folder where our C$ is set to.
 *
 *        The Likewise CIFS code maps C$ itself to /.
 *
 *        For instance, if the remote share was at the physical path
 *        C:\likewise, and our default share path was "/lwcifs", we would map
 *        the remote path "C:\likewise" to "C:\lwcifs\likewise".
 *
 */
DWORD
LwTaskGetLocalSharePathW(
	PWSTR  pwszSharePathWindows,
	PWSTR* ppwszSharePathLocal
	)
{
	DWORD     dwError = 0;
    wchar16_t wszBackslash[] = {'\\', 0};
    wchar16_t wszFwdslash[] = {'/', 0};
    wchar16_t wszColon[] = {':', 0};
    size_t    sSharePathWindowsLen = 0;
    size_t    sRequiredLen = 0;
    size_t    sFSPrefixLen = 3; // length("C:\");
    PWSTR     pwszPathReadCursor = pwszSharePathWindows;
    PWSTR     pwszPathWriteCursor = NULL;
    PWSTR     pwszSharePathLocal = NULL;
    PWSTR     pwszDefaultSharePath = NULL;

    if (IsNullOrEmptyString(pwszSharePathWindows))
	{
		dwError = ERROR_INVALID_PARAMETER;
		BAIL_ON_LW_TASK_ERROR(dwError);
	}

    pwszDefaultSharePath = gLwTaskGlobals.pwszDefaultSharePath;
    while (!IsNullOrEmptyString(pwszDefaultSharePath) &&
    	   ((*pwszDefaultSharePath == wszBackslash[0]) ||
    	    (*pwszDefaultSharePath == wszFwdslash[0])))
    {
    	pwszDefaultSharePath++;
    }

    if (IsNullOrEmptyString(pwszDefaultSharePath))
    {
    	dwError = LwAllocateWc16String(
    					&pwszSharePathLocal,
    					pwszSharePathWindows);
    	BAIL_ON_LW_TASK_ERROR(dwError);

    	goto done;
    }

    sSharePathWindowsLen = wc16slen(pwszSharePathWindows);

    if (((pwszSharePathWindows[1]  != wszColon[0]) &&
         !(pwszSharePathWindows[2] == wszBackslash[0] ||
           pwszSharePathWindows[2] == wszFwdslash[0])))
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_TASK_ERROR(dwError);
    }
    else
    {
        pwszPathReadCursor += sFSPrefixLen;
    }

    if ((wc16slen(pwszPathReadCursor) >= wc16slen(pwszDefaultSharePath)) &&
        !wc16scasecmp(pwszPathReadCursor, pwszDefaultSharePath))
    {
    	// Already have the required prefix ?

    	dwError = LwAllocateWc16String(
    					&pwszSharePathLocal,
    					pwszSharePathWindows);
    	BAIL_ON_LW_TASK_ERROR(dwError);

    	goto done;
    }

    sRequiredLen =  (wc16slen(pwszSharePathWindows) +
    				 wc16slen(pwszDefaultSharePath) +
    				 2 )* sizeof(wchar16_t);

    dwError = LwAllocateMemory(sRequiredLen, (PVOID*)&pwszSharePathLocal);
    BAIL_ON_LW_TASK_ERROR(dwError);

    pwszPathWriteCursor = pwszSharePathLocal;
    pwszPathReadCursor  = pwszSharePathWindows;

    memcpy(	(PBYTE)pwszPathWriteCursor,
    		(PBYTE)pwszPathReadCursor,
    		sFSPrefixLen * sizeof(wchar16_t));

    pwszPathWriteCursor += sFSPrefixLen;
    pwszPathReadCursor  += sFSPrefixLen;

    memcpy( (PBYTE)pwszPathWriteCursor,
    		(PBYTE)pwszDefaultSharePath,
    		wc16slen(pwszDefaultSharePath) * sizeof(wchar16_t));

    pwszPathWriteCursor += wc16slen(pwszDefaultSharePath);

    *pwszPathWriteCursor++ = wszBackslash[0];

    memcpy( (PBYTE)pwszPathWriteCursor,
    		(PBYTE)pwszPathReadCursor,
    		wc16slen(pwszPathReadCursor) * sizeof(wchar16_t));

    pwszPathWriteCursor = pwszSharePathLocal;
    while (!IsNullOrEmptyString(pwszPathWriteCursor))
    {
    	if (*pwszPathWriteCursor == wszFwdslash[0])
    	{
    		*pwszPathWriteCursor = wszBackslash[0];
    	}
    	pwszPathWriteCursor++;
    }

done:

	*ppwszSharePathLocal = pwszSharePathLocal;

cleanup:

	return dwError;

error:

	*ppwszSharePathLocal = NULL;

	LW_SAFE_FREE_MEMORY(pwszSharePathLocal);

	goto cleanup;
}

DWORD
LwTaskGetMappedSharePathW(
    PWSTR  pwszDriverPrefix,
    PWSTR  pwszInputPath,
    PWSTR* ppwszPath
    )
{
    DWORD     dwError = 0;
    wchar16_t wszBackslash[] = {'\\', 0};
    wchar16_t wszFwdslash[] = {'/', 0};
    wchar16_t wszColon[] = {':', 0};
    PWSTR     pwszPathReadCursor = pwszInputPath;
    PWSTR     pwszPathWriteCursor = NULL;
    PWSTR     pwszPath = NULL;
    size_t    sInputPathLen = 0;
    size_t    sFSPrefixLen = 3;
    size_t    sDriverPrefixLen = 0;
    size_t    sRequiredLen = 0;

    if (!pwszInputPath || !*pwszInputPath)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_TASK_ERROR(dwError);
    }
    sInputPathLen = wc16slen(pwszInputPath);

    sDriverPrefixLen = wc16slen(pwszDriverPrefix);

    if ((sInputPathLen < sFSPrefixLen) ||
        ((pwszInputPath[1] != wszColon[0]) &&
         !(pwszInputPath[2] == wszBackslash[0] ||
           pwszInputPath[2] == wszFwdslash[0])))
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_TASK_ERROR(dwError);
    }
    else
    {
        sRequiredLen += sDriverPrefixLen * sizeof(wchar16_t);
        pwszPathReadCursor += sFSPrefixLen;
    }

    if (!pwszPathReadCursor || !*pwszPathReadCursor)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_TASK_ERROR(dwError);
    }

    sRequiredLen += sizeof(wchar16_t); // path delimiter

    while (pwszPathReadCursor &&
           *pwszPathReadCursor &&
           ((*pwszPathReadCursor == wszBackslash[0]) ||
            (*pwszPathReadCursor == wszFwdslash[0])))
    {
        pwszPathReadCursor++;
    }

    // The rest of the path
    sRequiredLen += wc16slen(pwszPathReadCursor) * sizeof(wchar16_t);
    sRequiredLen += sizeof(wchar16_t);

    dwError = LwAllocateMemory(sRequiredLen, (PVOID*)&pwszPath);
    BAIL_ON_LW_TASK_ERROR(dwError);

    pwszPathReadCursor = pwszInputPath;
    pwszPathWriteCursor = pwszPath;

    pwszPathReadCursor += sFSPrefixLen;
    if (sDriverPrefixLen)
    {
        memcpy( (PBYTE)pwszPathWriteCursor,
                (PBYTE)pwszDriverPrefix,
                sDriverPrefixLen * sizeof(wchar16_t));
        pwszPathWriteCursor += sDriverPrefixLen;

        *pwszPathWriteCursor++ = wszBackslash[0];
    }

    while (pwszPathReadCursor &&
           *pwszPathReadCursor &&
           ((*pwszPathReadCursor == wszBackslash[0]) ||
            (*pwszPathReadCursor == wszFwdslash[0])))
    {
        pwszPathReadCursor++;
    }

    while (pwszPathReadCursor && *pwszPathReadCursor)
    {
        if (*pwszPathReadCursor == wszFwdslash[0])
        {
            *pwszPathWriteCursor++ = wszBackslash[0];
        }
        else
        {
            *pwszPathWriteCursor++ = *pwszPathReadCursor;
        }
        pwszPathReadCursor++;
    }

    pwszPathWriteCursor = pwszPath;
    while (*pwszPathWriteCursor)
    {
        if (*pwszPathWriteCursor == wszFwdslash[0])
        {
            *pwszPathWriteCursor = wszBackslash[0];
        }

        pwszPathWriteCursor++;
    }

    *ppwszPath = pwszPath;

cleanup:

    return dwError;

error:

    *ppwszPath = NULL;

    LW_SAFE_FREE_MEMORY(pwszPath);

    goto cleanup;
}
