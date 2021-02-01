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

#include <domainjoin.h>
#include <reg/lwreg.h>

DWORD
DeleteTreeFromRegistry(
    PCSTR pszPath
    )
{
    DWORD ceError = ERROR_SUCCESS;
    HANDLE hReg = (HANDLE)NULL;
    HKEY pRootKey = NULL;

    ceError = RegOpenServer(&hReg);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = RegOpenKeyExA(
                hReg,
                NULL,
                HKEY_THIS_MACHINE,
                0,
                KEY_ALL_ACCESS,
                &pRootKey);
    if (ceError)
    {
        DJ_LOG_ERROR( "Failed to open registry root key %s",HKEY_THIS_MACHINE);
        goto error;
    }

    ceError = RegDeleteTreeA(
                hReg,
                pRootKey,
                pszPath);
    if (ceError)
    {
        // Do NOT log if ceError is about the key which is not present
        ceError = ERROR_SUCCESS;
    }

cleanup:
    RegCloseKey(hReg, pRootKey);
    pRootKey = NULL;

    RegCloseServer(hReg);
    hReg = NULL;

    return(ceError);

error:
    goto cleanup;
}

DWORD
DeleteValueFromRegistry(
    PCSTR pszPath,
    PCSTR pszName
    )
{
    DWORD ceError = ERROR_SUCCESS;
    HANDLE hReg = (HANDLE)NULL;
    HKEY pRootKey = NULL;

    ceError = RegOpenServer(&hReg);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = RegOpenKeyExA(
                hReg,
                NULL,
                HKEY_THIS_MACHINE,
                0,
                KEY_ALL_ACCESS,
                &pRootKey);
    if (ceError)
    {
        DJ_LOG_ERROR( "Failed to open registry root key %s",HKEY_THIS_MACHINE);
        goto error;
    }

    ceError = RegDeleteKeyValueA(
                hReg,
                pRootKey,
                pszPath,
                pszName);

    if (ceError == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE || ceError == LWREG_ERROR_DELETE_DEFAULT_VALUE_NOT_ALLOWED)
    {
        ceError = ERROR_SUCCESS;
    }

cleanup:
    if (hReg)
    {
        if (pRootKey)
        {
            RegCloseKey(hReg, pRootKey);
            pRootKey = NULL;
        }

        RegCloseServer(hReg);
        hReg = NULL;
    }

    return(ceError);

error:
    goto cleanup;
}

DWORD
SetBooleanRegistryValue(
    PCSTR path,
    PCSTR name,
    BOOL  value
    )
{
    DWORD ceError = ERROR_SUCCESS;
    HANDLE hReg = (HANDLE)NULL;
    HKEY pRootKey = NULL;
    HKEY pNodeKey = NULL;
    DWORD dwValue = 0;

    if (value)
    {
        dwValue = 1;
    }

    ceError = RegOpenServer(&hReg);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = RegOpenKeyExA(
                hReg,
                NULL,
                HKEY_THIS_MACHINE,
                0,
                KEY_ALL_ACCESS,
                &pRootKey);
    if (ceError)
    {
        DJ_LOG_ERROR( "Failed to open registry root key %s",HKEY_THIS_MACHINE);
        goto error;
    }

    ceError = RegOpenKeyExA(
                hReg,
                pRootKey,
                path,
                0,
                KEY_ALL_ACCESS,
                &pNodeKey);
    if (ceError)
    {
        DJ_LOG_ERROR( "Failed to open registry key %s\\%s",HKEY_THIS_MACHINE, path);
        goto error;
    }

    ceError = RegSetValueExA(
                hReg,
                pNodeKey,
                name,
                0,
                REG_DWORD,
                (const BYTE*) &dwValue,
                sizeof(dwValue));
    if (ceError)
    {
        DJ_LOG_ERROR( "Failed to set registry value %s with value %d", name, value ? 1 : 0);
        goto error;
    }

cleanup:

    if (hReg)
    {
        if (pNodeKey)
        {
            RegCloseKey(hReg, pNodeKey);
            pNodeKey = NULL;
        }

        if (pRootKey)
        {
            RegCloseKey(hReg, pRootKey);
            pRootKey = NULL;
        }

        RegCloseServer(hReg);
        hReg = NULL;
    }

    return(ceError);

error:
    goto cleanup;
}

DWORD
SetDwordRegistryValue(
    PCSTR path,
    PCSTR name,
    DWORD value
    )
{
    DWORD ceError = ERROR_SUCCESS;
    HANDLE hReg = (HANDLE)NULL;
    HKEY pRootKey = NULL;
    HKEY pNodeKey = NULL;

    ceError = RegOpenServer(&hReg);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = RegOpenKeyExA(
                hReg,
                NULL,
                HKEY_THIS_MACHINE,
                0,
                KEY_ALL_ACCESS,
                &pRootKey);
    if (ceError)
    {
        DJ_LOG_ERROR( "Failed to open registry root key %s",HKEY_THIS_MACHINE);
        goto error;
    }

    ceError = RegOpenKeyExA(
                hReg,
                pRootKey,
                path,
                0,
                KEY_ALL_ACCESS,
                &pNodeKey);
    if (ceError)
    {
        DJ_LOG_ERROR( "Failed to open registry key %s\\%s",HKEY_THIS_MACHINE, path);
        goto error;
    }

    ceError = RegSetValueExA(
                hReg,
                pNodeKey,
                name,
                0,
                REG_DWORD,
                (const BYTE*) &value,
                sizeof(value));
    if (ceError)
    {
        DJ_LOG_ERROR( "Failed to set registry value %s with value %d", name, value ? 1 : 0);
        goto error;
    }

cleanup:

    if (hReg)
    {
        if (pNodeKey)
        {
            RegCloseKey(hReg, pNodeKey);
            pNodeKey = NULL;
        }

        if (pRootKey)
        {
            RegCloseKey(hReg, pRootKey);
            pRootKey = NULL;
        }

        RegCloseServer(hReg);
        hReg = NULL;
    }

    return(ceError);

error:
    goto cleanup;
}

DWORD
SetStringRegistryValue(
    PCSTR path,
    PCSTR name,
    PSTR  value
    )
{
    DWORD ceError = ERROR_SUCCESS;
    HANDLE hReg = (HANDLE)NULL;
    HKEY pRootKey = NULL;
    HKEY pNodeKey = NULL;
    char szEmpty[2] = "";

    if (!value)
    {
        value = szEmpty;
    }

    ceError = RegOpenServer(&hReg);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = RegOpenKeyExA(
                hReg,
                NULL,
                HKEY_THIS_MACHINE,
                0,
                KEY_ALL_ACCESS,
                &pRootKey);
    if (ceError)
    {
        DJ_LOG_ERROR( "Failed to open registry root key %s",HKEY_THIS_MACHINE);
        goto error;
    }

    ceError = RegOpenKeyExA(
                hReg,
                pRootKey,
                path,
                0,
                KEY_ALL_ACCESS,
                &pNodeKey);
    if (ceError)
    {
        DJ_LOG_ERROR( "Failed to open registry key %s\\%s",HKEY_THIS_MACHINE, path);
        goto error;
    }

    ceError = RegSetValueExA(
                hReg,
                pNodeKey,
                name,
                0,
                REG_SZ,
                (const BYTE*)value,
                (DWORD)strlen(value)+1);
    if (ceError)
    {
        DJ_LOG_ERROR( "Failed to set registry value %s with value %s", name, value);
        goto error;
    }


cleanup:

    if (hReg)
    {
        if (pNodeKey)
        {
            RegCloseKey(hReg, pNodeKey);
            pNodeKey = NULL;
        }

        if (pRootKey)
        {
            RegCloseKey(hReg, pRootKey);
            pRootKey = NULL;
        }

        RegCloseServer(hReg);
        hReg = NULL;
    }

    return(ceError);

error:
    goto cleanup;
}
