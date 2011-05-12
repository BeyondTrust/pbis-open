/*
 * dsprovider.h
 *
 *  Created on: Mar 9, 2009
 *      Author: krishnag
 */

#ifndef DSPROVIDER_H_
#define DSPROVIDER_H_

typedef DWORD (*PFNDIRECTORYOPEN)(
                    PHANDLE phDirectory
                    );

typedef DWORD (*PFNDIRECTORYBIND)(
                    HANDLE hDirectory,
                    PWSTR  pwszDistinguishedName,
                    PWSTR  pwszCredential,
                    ULONG  ulMethod
                    );

typedef DWORD (*PFNDIRECTORYADD)(
                    HANDLE hDirectory,
                    PWSTR  pwszObjectDN,
                    DIRECTORY_MOD Attributes[]
                    );

typedef DWORD (*PFNDIRECTORYMODIFY)(
                    HANDLE hDirectory,
                    PWSTR  pwszObjectDN,
                    DIRECTORY_MOD Modifications[]
                    );

typedef DWORD (*PFNDIRECTORYSETPASSWORD)(
                    HANDLE hDirectory,
                    PWSTR  pwszObjectDN,
                    PWSTR  pwszPassword
                    );

typedef DWORD (*PFNDIRECTORYCHANGEPASSWORD)(
                    HANDLE hDirectory,
                    PWSTR  pwszObjectDN,
                    PWSTR  pwszOldPassword,
                    PWSTR  pwszNewPassword
                    );

typedef DWORD (*PFNDIRECTORYVERIFYPASSWORD)(
                    HANDLE hDirectory,
                    PWSTR  pwszObjectDN,
                    PWSTR  pwszPassword
                    );

typedef DWORD (*PFNDIRECTORYGETMEMBERS)(
                    HANDLE            hDirectory,
                    PWSTR             pwszGroupDN,
                    PWSTR             pwszAttrs[],
                    PDIRECTORY_ENTRY* ppDirectoryEntries,
                    PDWORD            pdwNumEntries
                    );

typedef DWORD (*PFNDIRECTORYMANAGEMEMBER)(
                    HANDLE           hDirectory,
                    PWSTR            pwszGroupDN,
                    PDIRECTORY_ENTRY pDirectoryEntries
                    );

typedef DWORD (*PFNDIRECTORYSEARCH)(
                    HANDLE            hDirectory,
                    PWSTR             pwszBase,
                    ULONG             ulScope,
                    PWSTR             pwszFilter,
                    PWSTR             wszAttributes[],
                    ULONG             ulAttributesOnly,
                    PDIRECTORY_ENTRY* ppDirectoryEntries,
                    PDWORD            pdwNumEntries
                    );

typedef DWORD (*PFNDIRECTORYGETUSERCOUNT)(
                    HANDLE hDirectory,
                    PDWORD pdwNumUsers
                    );

typedef DWORD (*PFNDIRECTORYGETGROUPCOUNT)(
                    HANDLE hDirectory,
                    PDWORD pdwNumGroups
                    );

typedef DWORD (*PFNDIRECTORYDELETE)(
                    HANDLE hDirectory,
                    PWSTR  pwszObjectDN
                    );

typedef VOID (*PFNDIRECTORYCLOSE)(
                    HANDLE hDirectory
                    );

typedef struct __LSA_DIRECTORY_PROVIDER_FUNCTION_TABLE
{
    PFNDIRECTORYOPEN           pfnDirectoryOpen;
    PFNDIRECTORYBIND           pfnDirectoryBind;
    PFNDIRECTORYADD            pfnDirectoryAdd;
    PFNDIRECTORYMODIFY         pfnDirectoryModify;
    PFNDIRECTORYSETPASSWORD    pfnDirectorySetPassword;
    PFNDIRECTORYCHANGEPASSWORD pfnDirectoryChangePassword;
    PFNDIRECTORYVERIFYPASSWORD pfnDirectoryVerifyPassword;
    PFNDIRECTORYGETMEMBERS     pfnDirectoryGetGroupMembers;
    PFNDIRECTORYGETMEMBERS     pfnDirectoryGetMemberships;
    PFNDIRECTORYMANAGEMEMBER   pfnDirectoryAddToGroup;
    PFNDIRECTORYMANAGEMEMBER   pfnDirectoryRemoveFromGroup;
    PFNDIRECTORYDELETE         pfnDirectoryDelete;
    PFNDIRECTORYSEARCH         pfnDirectorySearch;
    PFNDIRECTORYGETUSERCOUNT   pfnDirectoryGetUserCount;
    PFNDIRECTORYGETGROUPCOUNT  pfnDirectoryGetGroupCount;
    PFNDIRECTORYCLOSE          pfnDirectoryClose;

} DIRECTORY_PROVIDER_FUNCTION_TABLE, *PDIRECTORY_PROVIDER_FUNCTION_TABLE;

#define DIRECTORY_SYMBOL_NAME_INITIALIZE_PROVIDER "DirectoryInitializeProvider"

typedef DWORD (*PFNINITIALIZEDIRPROVIDER)(
                    PSTR* ppszProviderName,
                    PDIRECTORY_PROVIDER_FUNCTION_TABLE* ppFnTable
                    );

#define DIRECTORY_SYMBOL_NAME_SHUTDOWN_PROVIDER "DirectoryShutdownProvider"

typedef DWORD (*PFNSHUTDOWNDIRPROVIDER)(
                    PSTR pszProviderName,
                    PDIRECTORY_PROVIDER_FUNCTION_TABLE pFnTable
                    );

DWORD
DirectoryInitializeProvider(
    PSTR* ppszProviderName,
    PDIRECTORY_PROVIDER_FUNCTION_TABLE* ppFnTable
    );

DWORD
DirectoryShutdownProvider(
    PSTR pszProviderName,
    PDIRECTORY_PROVIDER_FUNCTION_TABLE pFnTable
    );

#endif /* DSPROVIDER_H_ */
