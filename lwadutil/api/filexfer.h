#ifndef __ADUFILEXFER_H__
#define __ADUFILEXFER_H__

DWORD
ADUCrackFileSysPath(
    PCSTR  pszFileSysPath,
    PSTR * ppszDomainName,
    PSTR * ppszSourcePath,
    PSTR * ppszPolicyIdentifier
    );

DWORD
ADUSMBGetFile(
    PSTR  pszDomainName,
    PSTR  pszSourcePath,
    PSTR  pszDestPath
    );

DWORD
ADUSMBPutFile(
    PSTR  pszDomainName,
    PSTR  pszSourceFolder,
    PSTR  pszFileName,
    PSTR  pszDestFolder
    );

DWORD
ADUSMBGetFolder(
    PSTR  pszDomainName,
    PSTR  pszSourceFolder,
    PSTR  pszDestFolder
    );

DWORD
ADUSMBPutFolder(
    PSTR    pszDomainName,
    PSTR    pszSourceFolderParent,
    PSTR    pszFolderName,
    PSTR    pszDestFolder,
    BOOLEAN fReplace
    );

#endif /* __ADUFILEXFER_H__ */
