#ifndef LWTEST_UTILS
#define LWTEST_UTILS
#include "lwteststructs.h"

DWORD 
Csv_LoadInterface(
    PLWTDATAIFACE *ppLwtDataIface
    );

DWORD
Ldif_LoadInterface(
    PLWTDATAIFACE *ppLwtDataIface
    );



/* utility.c */
BOOL
StringsAreEqual(
    PCSTR,
    PCSTR
    );

BOOL
StringsNoCaseAreEqual(
    PCSTR,
    PCSTR
    );

void
Lwt_strcat(
    char *str,
    size_t size,
    const char *addition
    );

void
LwtLogTest(
    PCSTR pszFunction,
    PCSTR pszFile,
    PCSTR pszTestDescription,
    PCSTR pszTestAPIs,
    DWORD dwError,
    PCSTR pszMsg
    );

DWORD
LwtMapErrorToProgramStatus(
    DWORD dwError
    );


DWORD
LwtMapErrorToProgramStatus(
    DWORD dwError
    );



DWORD
LwtInitLogging(
    PCSTR    pszPath,
    int      nAppend,
    int      nLogLevel
    );



VOID
LwtShutdownLogging(
    VOID
   );


/*
 * InitializeGroupInfo
 * 
 * Function opens the group information files and update the file information to PLWTCSV structure 
 * for future use
 * 
 */
DWORD
InitializeGroupInfo(
    PCSTR pszCsvFilename,
    PLWTDATAIFACE *ppGroupIface,
    DWORD* pdwNumGroups
    );

/*
 * DestroyGroupInfo
 * 
 * Function closes the group information file and frees the memory allocated for PLWTCSV structure
 * 
 */
void
DestroyGroupInfo(
    PLWTDATAIFACE  pGroupIface
    );


/*
 * GetGroup
 * 
 * Function reads specified row from the group information file and update to group structure.
 * 
 */
DWORD
GetGroup(
    PTESTDATA pTestContext,
    size_t index,
    PLWTGROUP *ppGroup
    );

/*
 * FreeGroup 
 * 
 * Function frees the memory allocated for group and its mebers.
 * 
 */
void
FreeGroup(
    PLWTGROUP *ppGroup
    );



/* 
* InitialiseUserInfo
*
* Initialize user info structure 
*/
DWORD
InitialiseUserInfo(
    PCSTR pCsvFilename, 
    PLWTDATAIFACE *ppUserIface,
    DWORD *pdwNumUsers
    );


/*
* Frees User Info structure
*/
DWORD
DestroyUserInfo(
    PLWTDATAIFACE pvUserIface
    );




/*
* Gets the user info details
*/
DWORD
GetUser(
    PTESTDATA pTestData,
    size_t index,
    PLWTUSER *ppUser
    );


/*
*
* Get the invalid data records
*
*/

DWORD
GetInvalidDataRecord(
    PTESTDATA pTestData,
    size_t index,
    PLWTFAILDATA *ppUser
    );


/*
* open CSV file and keep the information in a structure
* for later access
*/
DWORD
InitialiseInvalidDataSet(
    PCSTR pInputFile,
    PLWTDATAIFACE *ppDataIface,
    DWORD *pdwNumUsers
    );


/*
* Free invalid data record
*/
VOID
FreeInvalidDataRecord(
    PLWTFAILDATA pData
    );



VOID
FreeUser(
    PLWTUSER *ppUser
    );



DWORD
Lwt_LsaTestGetValue(
    PLWTDATA pData,
    PCSTR pszField,
    PSTR *ppSzValue
);

VOID
Lwt_LsaTestFreeData(
    PLWTDATA pData
    );

#endif
