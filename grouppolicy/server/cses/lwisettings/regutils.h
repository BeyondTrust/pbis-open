#ifndef __REGUTILS_H__
#define __REGUTILS_H__

CENTERROR
GPSetStringToRegistry(
    HANDLE hConnection, 
    HKEY kKey, 
    PCSTR pszValueName, 
    PSTR pszValue
    );

CENTERROR
GPSetMultiStringToRegistry(
    HANDLE hConnection, 
    HKEY kKey, 
    PCSTR pszValueName, 
    PSTR pszValue
    );

CENTERROR
GPSetBoolToRegistry(
    HANDLE hConnection, 
    HKEY kKey, 
    PCSTR pszValueName, 
    BOOL bValue
    );

CENTERROR
GPSetDwordToRegistry(
    HANDLE hConnection, 
    HKEY kKey, 
    PCSTR pszValueName, 
    DWORD dwValue
    );

CENTERROR
GPGetRegOpenKey(
    PSTR pszKeyPath,
    PHANDLE phConnection,
    PHKEY   phKey
    );

CENTERROR
GPDeleteValueFromRegistry(
    HANDLE hConnection, 
    HKEY hKey, 
    PCSTR pszValueName
    );

CENTERROR
UpdateDwordValueInRegistry( 
    HANDLE hReg,
    HKEY hKey, 
    BOOLEAN bUseDefault, 
    PCSTR pszValueName,
    DWORD dwValue
    );

CENTERROR
UpdateBoolValueInRegistry( 
    HANDLE hReg,
    HKEY hKey, 
    BOOLEAN bUseDefault, 
    PCSTR pszValueName,
    BOOL bValue
    );

CENTERROR
UpdateStringValueInRegistry( 
    HANDLE hReg,
    HKEY hKey, 
    BOOLEAN bUseDefault, 
    PCSTR pszValueName,
    PSTR pszValue
    );

CENTERROR
UpdateMultiStringValueInRegistry( 
    HANDLE hReg,
    HKEY hKey, 
    BOOLEAN bUseDefault, 
    PCSTR pszValueName,
    PSTR pszValue
    );

BOOL
GetBoolValue( 
    PSTR pszValue
    );

#endif
