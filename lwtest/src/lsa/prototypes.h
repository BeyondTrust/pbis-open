
/* lwt_lsa_framework.c */
DWORD
Lwt_LsaTestSetup(
    int argc,
    char *argv[],
    HANDLE *phLsaConnection,
    PTESTDATA *ppTestData
    );


DWORD
Lwt_LsaTestTeardown(
    HANDLE *phLsaConnection,
    PTESTDATA *ppTestData
    );


DWORD
Lwt_LsaEnumUsers(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    );

DWORD
Lwt_LsaFindUserByName(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    );

/*
 *
 * Authenticate for valid user
 *
 */
DWORD
AuthenticateUserValid(
    HANDLE hLsaConnection,
    PSTR pszUser,
    PSTR pszPassword
    );

/*
 *
 * Authenticate for disabled user
 *
 */
DWORD
AuthenticateUserDisabled(
    HANDLE hLsaConnection,
    PSTR pszUser,
    PSTR pszPassword
    );

/*
 *
 *  Authenticate for the locked out user
 *
 */
DWORD
AuthenticateUserLockedOut(
    HANDLE hLsaConnection,
    PSTR pszUser,
    PSTR pszPassword
    );

/*
 *
 * High level API to enumerate the groups in the AD
 *
 */
int 
Lwt_LsaBeginEnumerationGroups( 
    HANDLE hLSAServer,
    DWORD dwLevel,
    PVOID* ppGroupInfo,
    DWORD* dwMaxGroups,
    BOOL bOnline
);   

/*
 * CheckLsaEnumUsers
 * 
 * Check LSA_USER_INFO_* list from LsaEnumUsers has expected user.
 */
DWORD
CheckLsaEnumUsers(
    HANDLE hLsaConnection,
    PCSTR pszUser,
    DWORD dwUserInfoLevel,
    DWORD dwMaxNumUsers
    );

/*
 *
 * Validation for valid user
 *
 */
DWORD
ValidateUserValid(
    HANDLE hLsaConnection,
    PSTR pszUser,
    PSTR pszPassword
    );

/*
 *
 * Validation for disabled user
 *
 */
DWORD
ValidateUserDisabled(
    HANDLE hLsaConnection,
    PSTR pszUser,
    PSTR pszPassword
    );

/*
 *
 *  Validation for locked out user
 *
 */
DWORD
ValidateUserLockedOut(
    HANDLE hLsaConnection,
    PSTR pszUser,
    PSTR pszPassword
    );

/*
 *
 * Verify LSA_SID_INFO  
 * against info in csv file
 * 
 */
DWORD
VerifySidInformation(
    PLSA_SID_INFO pSIDInfoList,
    PCSTR pszSamAccountName,
    PCSTR pszDomainName,
    DWORD dwAccountType
    );
    
