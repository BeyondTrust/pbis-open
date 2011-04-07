/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
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
        //Donot log if ceError is about the key which is not present
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
