#include "includes.h"

#define STATIC_PATH_BUFFER_SIZE 256

CENTERROR
GPOGetUserCachePath(
    uid_t uid,
    PSTR * ppszCachePath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszCachePath = NULL;

    ceError = LwAllocateStringPrintf(&pszCachePath,
                                     "/tmp/krb5cc_%ld",
                                     (long)uid);
    BAIL_ON_CENTERIS_ERROR(ceError);

    *ppszCachePath = pszCachePath;

cleanup:

    return ceError;

error:

    *ppszCachePath = NULL;

    goto cleanup;
}

CENTERROR
GPOGetSystemCachePath(
    PSTR * ppszCachePath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR  pszCachePath = NULL;

    ceError = LwAllocateStringPrintf(&pszCachePath,
                                     "%s/krb5cc_gpagentd",
                                     CACHEDIR);
    BAIL_ON_CENTERIS_ERROR(ceError);

    *ppszCachePath = pszCachePath;

cleanup:

    return ceError;

error:

    *ppszCachePath = NULL;

    goto cleanup;
}

CENTERROR
GPOSetCachePath(
    PSTR  pszCachePath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    DWORD dwMajorStatus = 0;
    DWORD dwMinorStatus = 0;

    dwMajorStatus = gss_krb5_ccache_name(
                            (OM_uint32 *)&dwMinorStatus,
                            pszCachePath,
                            NULL);
    if (dwMajorStatus)
    {
        GPA_LOG_ERROR("Failed to set Kerberos cred cache to (path: %s), gss error (dwMajorStatus: %d, dwMinorStatus: %d)", pszCachePath, dwMajorStatus, dwMinorStatus);
        ceError = CENTERROR_GP_GSS_CALL_FAILED;                 \
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

cleanup:

    return ceError;

error:

    goto cleanup;
}

CENTERROR
GPOLwioCopyFile(
    PGPUSER pUser,
    PSTR pszDomainName,
    PSTR pszPreferredServerIP, /* Optional */
    PSTR pszSourcePath,
    PSTR pszDestPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR  szSourcePath[2 * PATH_MAX + 1];
    PSTR  pszCachePath = NULL;
    PSTR  pszPrincipal = NULL;
    LW_PIO_CREDS pExistingCreds = NULL;
    LW_PIO_CREDS pImpersonateCreds = NULL;
    LW_NTSTATUS status = 0;

    if (pUser)
    {
        ceError = GPOGetUserCachePath(pUser->uid, &pszCachePath);
        if (status)
        {
            ceError = CENTERROR_GP_FILE_COPY_FAILED;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        ceError = GPAGetPrincipalFromKrb5Cache(pszCachePath, &pszPrincipal);
        BAIL_ON_CENTERIS_ERROR(ceError);

        // Get current creds token, so we can restore later
        status = LwIoGetThreadCreds(&pExistingCreds);
        if (status)
        {
            ceError = CENTERROR_GP_FILE_COPY_FAILED;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        // Create new creds token to impersonate a given user
        status = LwIoCreateKrb5CredsA(pszPrincipal,
                                      pszCachePath,
                                      &pImpersonateCreds);
        if (status)
        {
            ceError = CENTERROR_GP_FILE_COPY_FAILED;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        status = LwIoSetThreadCreds(pImpersonateCreds);
        if (status)
        {
            ceError = CENTERROR_GP_FILE_COPY_FAILED;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }

    sprintf(szSourcePath, "/%s/sysvol/%s",
            pszPreferredServerIP ? pszPreferredServerIP : pszDomainName,
            pszSourcePath);

    GPA_LOG_VERBOSE("Copying from the path: %s", szSourcePath);

    ceError = GPACopyFileFromRemote(szSourcePath,pszDestPath);
    if (ceError == CENTERROR_GP_PATH_NOT_FOUND ||
        ceError == CENTERROR_GP_CREATE_FAILED)
    {
        goto error;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    if (pExistingCreds)
    {
        // Revert back to original credentials for the given thread.
        LwIoSetThreadCreds(pExistingCreds);
        LwIoDeleteCreds(pExistingCreds);
    }

    if (pImpersonateCreds)
    {
        LwIoDeleteCreds(pImpersonateCreds);
    }

    LW_SAFE_FREE_STRING(pszCachePath);
    LW_SAFE_FREE_STRING(pszPrincipal);

    return(ceError);
}

CENTERROR
GPOLwioCopyFolder(
    PGPUSER pUser,
    PSTR pszDomainName,
    PSTR pszPreferredServerIP, /* Optional */
    PSTR pszSourceFolder,
    PSTR pszDestFolder
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR  szSourcePath[PATH_MAX + 1];
    PSTR  pszSourceDirname = NULL;
    PSTR  pszSourceBasename = NULL;
    PSTR  pszCachePath = NULL;
    PSTR  pszPrincipal = NULL;
    LW_PIO_CREDS pExistingCreds = NULL;
    LW_PIO_CREDS pImpersonateCreds = NULL;
    LW_NTSTATUS status = 0;

    if (IsNullOrEmptyString(pszDomainName) ||
        IsNullOrEmptyString(pszSourceFolder) ||
        IsNullOrEmptyString(pszDestFolder) )
    {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (pUser)
    {
        ceError = GPOGetUserCachePath(pUser->uid, &pszCachePath);
        if (status)
        {
            ceError = CENTERROR_GP_FILE_COPY_FAILED;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        ceError = GPAGetPrincipalFromKrb5Cache(pszCachePath, &pszPrincipal);
        BAIL_ON_CENTERIS_ERROR(ceError);

        // Get current creds token, so we can restore later
        status = LwIoGetThreadCreds(&pExistingCreds);
        if (status)
        {
            ceError = CENTERROR_GP_FILE_COPY_FAILED;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        // Create new creds token to impersonate a given user
        status = LwIoCreateKrb5CredsA(pszPrincipal,
                                      pszCachePath,
                                      &pImpersonateCreds);
        if (status)
        {
            ceError = CENTERROR_GP_FILE_COPY_FAILED;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        status = LwIoSetThreadCreds(pImpersonateCreds);
        if (status)
        {
            ceError = CENTERROR_GP_FILE_COPY_FAILED;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }

    ceError = GPOGetSysDirBase( pszSourceFolder, &pszSourceDirname, &pszSourceBasename );    
    BAIL_ON_CENTERIS_ERROR(ceError);    
    
    sprintf(szSourcePath, "/%s/sysvol/%s",
            pszPreferredServerIP ? pszPreferredServerIP : pszDomainName,
            pszSourceFolder);

    GPA_LOG_VERBOSE("Copying from the path: %s", szSourcePath);

    ceError = GPACopyDirFromRemote(szSourcePath,pszDestFolder);
    if (ceError == CENTERROR_GP_PATH_NOT_FOUND ||
        ceError == CENTERROR_GP_CREATE_FAILED)
    {
        goto error;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);


error:

    if (pExistingCreds)
    {
        // Revert back to original credentials for the given thread.
        LwIoSetThreadCreds(pExistingCreds);
        LwIoDeleteCreds(pExistingCreds);
    }

    if (pImpersonateCreds)
    {
        LwIoDeleteCreds(pImpersonateCreds);
    }

    LW_SAFE_FREE_STRING(pszCachePath);
    LW_SAFE_FREE_STRING(pszPrincipal);
    LW_SAFE_FREE_STRING(pszSourceDirname);
    LW_SAFE_FREE_STRING(pszSourceBasename);

    return ceError;
}

CENTERROR
GPOGetPolicyFiles(
    PGPUSER pUser,
    DWORD dwPolicyType,
    PSTR pszgGpSysVolPath,
    PSTR pszgCseIdentifier,
    PSTR pszDestFolderRootPath,
    PSTR *ppszDestFolder,
    BOOLEAN *pbPolicyExists
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    char szSourceFolderPath[STATIC_PATH_BUFFER_SIZE] = {0};  
    char szDestFolderPath[STATIC_PATH_BUFFER_SIZE] = {0};
    char szDestGPFolderPath[STATIC_PATH_BUFFER_SIZE] = {0};
    PSTR pszDomainName = NULL;
    PSTR pszSourcePath = NULL;
    PSTR pszPolicyIdentifier = NULL;
    BOOLEAN bFolderExists = FALSE;
    PSTR pszDC = NULL;  

    ceError = GPOCrackFileSysPath(pszgGpSysVolPath,
                                  &pszDomainName,
                                  &pszSourcePath,
                                  &pszPolicyIdentifier);
    BAIL_ON_CENTERIS_ERROR(ceError);
  
    ceError = GPAGetPreferredDomainController(pszDomainName, &pszDC);
    if (!CENTERROR_IS_OK(ceError))
    {
        if (CENTERROR_EQUAL(ceError, CENTERROR_GP_NO_SMB_KRB5_SITE_KDC_INFO) ||
            CENTERROR_EQUAL(ceError, CENTERROR_GP_NO_SMB_KRB5_SITE_INFO))
        {
           GPA_LOG_ALWAYS("GPAgent unable to obtain preferred server for AD site: FQDN(%s)", pszDomainName);
           ceError = CENTERROR_SUCCESS;
        }
        else
        {
           BAIL_ON_CENTERIS_ERROR(ceError);
        }
    } 

    if (pUser != NULL)
    {
        sprintf(szDestGPFolderPath,
                "%s/user-cache/%ld/%s",
                IsNullOrEmptyString(pszDestFolderRootPath) ? CACHEDIR : pszDestFolderRootPath,
                (long int)pUser->uid,
                pszPolicyIdentifier);
    }
    else
    {
        sprintf(szDestGPFolderPath,
                "%s/%s",
                IsNullOrEmptyString(pszDestFolderRootPath) ? CACHEDIR : pszDestFolderRootPath,
                pszPolicyIdentifier);
    }

    if ( pszgCseIdentifier != NULL )
    {
        sprintf(szSourceFolderPath,
                "%s\\%s\\Centeris\\Identity\\%s",
                pszSourcePath,
                (dwPolicyType == MACHINE_GROUP_POLICY ? "Machine" : "User"),
                pszgCseIdentifier );

        if (pUser != NULL)
        {
            sprintf(szDestFolderPath,
                    "%s/user-cache/%ld/%s",
                    szDestGPFolderPath,
                    (long int)pUser->uid,
                    pszgCseIdentifier);
        }
        else
        {
            sprintf(szDestFolderPath,
                    "%s/%s",
                    szDestGPFolderPath,
                    pszgCseIdentifier);
        }
    }
    else
    {
        sprintf(szSourceFolderPath, "%s\\%s\\Centeris\\Identity", pszSourcePath, (dwPolicyType == MACHINE_GROUP_POLICY ? "Machine" : "User") );
    }

    ceError = GPACheckDirectoryExists(szDestFolderPath, &bFolderExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFolderExists)
    {
        ceError = GPARemoveDirectory(szDestFolderPath);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }          

    ceError = GPOLwioCopyFolder(pUser,
                                pszDomainName,
                                pszDC,
                                szSourceFolderPath,
                                szDestGPFolderPath );
    if (ceError == CENTERROR_GP_PATH_NOT_FOUND ||
        ceError == CENTERROR_GP_CREATE_FAILED)
    {
        *pbPolicyExists = FALSE;
        goto error;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPACheckDirectoryExists(szDestFolderPath, pbPolicyExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if ( *pbPolicyExists )
    {
        GPA_LOG_VERBOSE("Received policy: Domain Name [%s] Source Path [%s] Dest Path [%s]", pszDomainName, szSourceFolderPath, szDestFolderPath);

        ceError = LwAllocateString( szDestFolderPath, ppszDestFolder );
        BAIL_ON_CENTERIS_ERROR(ceError);      
    }

error:

    if (pszDomainName)
    {
        LwFreeString(pszDomainName);
    }

    if (pszSourcePath)
    {
        LwFreeString(pszSourcePath);
    }

    if (pszPolicyIdentifier)
    {
        LwFreeString(pszPolicyIdentifier);
    }

    if (pszDC)
    {
        LwFreeString(pszDC);
    }

    return(ceError);
}

CENTERROR
GPOLwioCopyFileMultiple(
    PGPUSER pUser,
    PSTR pszDomainName,
    PSTR pszPreferredServerIP, /* Optional */
    PSTR pszSourceFolder,
    PSTR pszDestFolder
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR  szSourcePath[2 * PATH_MAX + 1];
    PSTR  pszCachePath = NULL;
    PSTR  pszPrincipal = NULL;
    BOOLEAN bFolderExists = 0;
    LW_PIO_CREDS pExistingCreds = NULL;
    LW_PIO_CREDS pImpersonateCreds = NULL;
    LW_NTSTATUS status = 0;

    if (IsNullOrEmptyString(pszDomainName) ||
        IsNullOrEmptyString(pszSourceFolder) ||
        IsNullOrEmptyString(pszDestFolder))
    {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = GPACheckDirectoryExists(pszDestFolder, &bFolderExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!bFolderExists)
    {
        GPA_LOG_VERBOSE("Unable to copy files as %s directory is not present", pszDestFolder);
        goto error;
    }          

    if (pUser)
    {
        ceError = GPOGetUserCachePath(pUser->uid, &pszCachePath);
        if (status)
        {
            ceError = CENTERROR_GP_FILE_COPY_FAILED;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        ceError = GPAGetPrincipalFromKrb5Cache(pszCachePath, &pszPrincipal);
        BAIL_ON_CENTERIS_ERROR(ceError);

        // Get current creds token, so we can restore later
        status = LwIoGetThreadCreds(&pExistingCreds);
        if (status)
        {
            ceError = CENTERROR_GP_FILE_COPY_FAILED;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        // Create new creds token to impersonate a given user
        status = LwIoCreateKrb5CredsA(pszPrincipal,
                                      pszCachePath,
                                      &pImpersonateCreds);
        if (status)
        {
            ceError = CENTERROR_GP_FILE_COPY_FAILED;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        status = LwIoSetThreadCreds(pImpersonateCreds);
        if (status)
        {
            ceError = CENTERROR_GP_FILE_COPY_FAILED;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }

    sprintf(szSourcePath, "/%s/sysvol/%s",
            pszPreferredServerIP ? pszPreferredServerIP : pszDomainName,
            pszSourceFolder);

    GPA_LOG_VERBOSE("Copy Command: %s", szSourcePath);

    ceError = GPACopyMultipleFilesFromRemote(szSourcePath,pszDestFolder);
    if (ceError == CENTERROR_GP_PATH_NOT_FOUND ||
        ceError == CENTERROR_GP_CREATE_FAILED)
    {
        goto error;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    if (pExistingCreds)
    {
        // Revert back to original credentials for the given thread.
        LwIoSetThreadCreds(pExistingCreds);
        LwIoDeleteCreds(pExistingCreds);
    }

    if (pImpersonateCreds)
    {
        LwIoDeleteCreds(pImpersonateCreds);
    }

    LW_SAFE_FREE_STRING(pszCachePath);
    LW_SAFE_FREE_STRING(pszPrincipal);

    return ceError;
}

CENTERROR
GPOGetPolicyFile(
    PGPUSER pUser,
    DWORD   dwPolicyType,
    PSTR    pszgGpSysVolPath,
    PSTR    pszgCseIdentifier,
    PSTR    pszgFileName,
    PSTR    pszDestFolderRootPath,
    PSTR    *ppszDestFile,
    BOOLEAN *pbFileExists
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    char szSourceFilePath[STATIC_PATH_BUFFER_SIZE];
    char szDestFolderPath[STATIC_PATH_BUFFER_SIZE];
    char szDestFilePath[STATIC_PATH_BUFFER_SIZE];
    PSTR pszDomainName = NULL;
    PSTR pszDC = NULL;
    PSTR pszSourcePath = NULL;
    PSTR pszPolicyIdentifier = NULL;
    BOOLEAN bExists = FALSE;

    memset(szSourceFilePath, 0, sizeof(szSourceFilePath));
    memset(szDestFilePath, 0, sizeof(szDestFilePath));
    memset(szDestFolderPath, 0, sizeof(szDestFolderPath));
  
    ceError =  GPOCrackFileSysPath(pszgGpSysVolPath,
                                   &pszDomainName,
                                   &pszSourcePath,
                                   &pszPolicyIdentifier);
    BAIL_ON_CENTERIS_ERROR(ceError);
  
    ceError = GPAGetPreferredDomainController(pszDomainName, &pszDC);
    if (!CENTERROR_IS_OK(ceError))
    {
        if (CENTERROR_EQUAL(ceError, CENTERROR_GP_NO_SMB_KRB5_SITE_KDC_INFO) ||
            CENTERROR_EQUAL(ceError, CENTERROR_GP_NO_SMB_KRB5_SITE_INFO))
        {
            GPA_LOG_ALWAYS("GPAgent unable to obtain preferred server for AD site: FQDN(%s)", pszDomainName );
            ceError = CENTERROR_SUCCESS;
        }
        else
        {
           BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }

    if ( pszgCseIdentifier != NULL )
    {
        sprintf(szSourceFilePath,
                "%s\\%s\\Centeris\\Identity\\%s\\%s",
                pszSourcePath,
                (dwPolicyType == MACHINE_GROUP_POLICY ? "Machine" : "User"),
                pszgCseIdentifier,
                pszgFileName );

        if (pUser != NULL)
        {
            sprintf(szDestFolderPath,
                    "%s/user-cache/%ld/%s/%s",
                    IsNullOrEmptyString(pszDestFolderRootPath) ? CACHEDIR : pszDestFolderRootPath,
                    (long int)pUser->uid,
                    pszPolicyIdentifier,
                    pszgCseIdentifier);
        }
        else
        {
            sprintf(szDestFolderPath,
                    "%s/%s/%s",
                    IsNullOrEmptyString(pszDestFolderRootPath) ? CACHEDIR : pszDestFolderRootPath,
                    pszPolicyIdentifier,
                    pszgCseIdentifier);
        }
    }
    else
    {
        sprintf(szSourceFilePath,
                "%s\\%s\\Centeris\\Identity\\%s",
                pszSourcePath,
                (dwPolicyType == MACHINE_GROUP_POLICY ? "Machine" : "User"),
                pszgFileName );
    
        if (pUser != NULL)
        {
            sprintf(szDestFolderPath,
                    "%s/user-cache/%ld/%s",
                    IsNullOrEmptyString(pszDestFolderRootPath) ? CACHEDIR : pszDestFolderRootPath,
                    (long int)pUser->uid,
                    pszPolicyIdentifier);
        }
        else
        {
            sprintf(szDestFolderPath,
                    "%s/%s",
                    IsNullOrEmptyString(pszDestFolderRootPath) ? CACHEDIR : pszDestFolderRootPath,
                    pszPolicyIdentifier);
        }
    }

    sprintf(szDestFilePath,
            "%s/%s.tmp",
            szDestFolderPath,
            pszgFileName);

    ceError = GPACheckDirectoryExists(szDestFolderPath, &bExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!bExists)
    {
      ceError = LwCreateDirectory(szDestFolderPath,
                                  S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
      BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = GPACheckFileExists(szDestFilePath, &bExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bExists)
    {
        ceError = LwRemoveFile( szDestFilePath );
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = GPOLwioCopyFile(pUser,
                              pszDomainName,
                              pszDC,
                              szSourceFilePath,
                              szDestFilePath);
    if (ceError == CENTERROR_GP_PATH_NOT_FOUND ||
        ceError == CENTERROR_GP_CREATE_FAILED)
    {
        *pbFileExists = FALSE;
        goto error;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPACheckFileExists(szDestFilePath, pbFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if ( *pbFileExists )
    {
        GPA_LOG_VERBOSE("Received policy file: Domain Name [%s] Source Path [%s] Dest Folder [%s]", pszDomainName, szSourceFilePath, szDestFolderPath);

        ceError = LwAllocateStringPrintf(ppszDestFile, 
                                         "%s/%s", 
                                         szDestFolderPath, 
                                         pszgFileName);
        BAIL_ON_CENTERIS_ERROR(ceError);

        LwRemoveFile( *ppszDestFile );

        GPACopyFileWithPerms(szDestFilePath, 
                            *ppszDestFile,
                            S_IRUSR|S_IRGRP);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = LwRemoveFile( szDestFilePath );
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    if (pszDomainName)
    {
        LwFreeString(pszDomainName);
    }

    if (pszDC)
    {
        LwFreeString(pszDC);
    }

    if (pszSourcePath)
    {
        LwFreeString(pszSourcePath);
    }

    if (pszPolicyIdentifier)
    {
        LwFreeString(pszPolicyIdentifier);
    }    

    return(ceError);
}

