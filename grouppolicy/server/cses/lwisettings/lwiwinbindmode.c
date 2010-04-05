#include "includes.h"

static PAD_SETTING gpHADSetting = NULL;

CENTERROR
AddSettings (
    PSTR pszName,
    PSTR pszValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PAD_SETTING pADSetting = gpHADSetting;
    PAD_SETTING pTmpADSetting = NULL;

    ceError = LwAllocateMemory( sizeof(AD_SETTING),
                                (PVOID *)&pTmpADSetting);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateString( pszName,
                               &pTmpADSetting->pszName);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateString( pszValue,
                               &pTmpADSetting->pszValue);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if(gpHADSetting == NULL) {
        gpHADSetting = pTmpADSetting;
    } else {
        while(pADSetting->pNext != NULL) {
            pADSetting = pADSetting ->pNext;
        }
        pADSetting->pNext = pTmpADSetting;
    }

error:

    return ceError;
}

VOID
FreeADSetting()
{
    PAD_SETTING pADSetting = gpHADSetting;
    PAD_SETTING pTmpADSetting = NULL;

    while(pADSetting) {
        pTmpADSetting = pADSetting;
        pADSetting = pADSetting->pNext;

        LW_SAFE_FREE_STRING(pTmpADSetting->pszName);
        LW_SAFE_FREE_STRING(pTmpADSetting->pszValue);
        LwFreeMemory(pTmpADSetting);
    }
    gpHADSetting = NULL;
}

#if defined(__LWI_SOLARIS__)
static
CENTERROR
GetSMBConfFilePathOnSolaris(
    PSTR *ppszCommand,
    PSTR *ppszFilePath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bFileExists = FALSE;

    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                (PVOID *)ppszCommand);
    BAIL_ON_CENTERIS_ERROR(ceError);

    /* /usr/local/samba/sbin/smbd */
    ceError = GPACheckFileExists( LWI_SOLARIS_SMBD_PATH_LOCAL,
                                 &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists) {
        sprintf( *ppszCommand,
                 "%s -b | grep CONFIGFILE",
                 LWI_SOLARIS_SMBD_PATH_LOCAL);
    } else {
        /* /usr/sfw/sbin/smbd */
        ceError = GPACheckFileExists( LWI_SOLARIS_SMBD_PATH_SFW,
                                     &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (bFileExists) {
            sprintf( *ppszCommand,
                     "%s -b | grep CONFIGFILE",
                     LWI_SOLARIS_SMBD_PATH_SFW);
        } else {
            /* /opt/csw/sbin/smbd */
            ceError = GPACheckFileExists( LWI_SOLARIS_SMBD_PATH_CSW,
                                         &bFileExists);
            BAIL_ON_CENTERIS_ERROR(ceError);

            if (bFileExists) {
                sprintf( *ppszCommand,
                         "%s -b | grep CONFIGFILE",
                         LWI_SOLARIS_SMBD_PATH_CSW);
            } else {
                ceError = GPACheckFileExists( "/usr/local/samba/lib/smb.conf",
                                             &bFileExists);
                BAIL_ON_CENTERIS_ERROR(ceError);

                if (bFileExists) {
                    ceError = LwAllocateStringPrintf( ppszFilePath,
                                                      "%s",
                                                      "/usr/local/samba/lib/smb.conf");
                    BAIL_ON_CENTERIS_ERROR(ceError);
                } else {
                    GPA_LOG_ALWAYS("Could not locate smb.conf system file, hence skipping the setting.");
                    return ceError;
                }
            }
        }
    }

error:

    return ceError;
}
#elif defined(__LWI_AIX__)
static
CENTERROR
GetSMBConfFilePathOnAIX(
    PSTR *ppszCommand,
    PSTR *ppszFilePath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bFileExists = FALSE;

    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                (PVOID *)ppszCommand);
    BAIL_ON_CENTERIS_ERROR(ceError);

    /* /usr/local/sbin/smbd */
    ceError = GPACheckFileExists( "/usr/local/sbin/smbd",
                                 &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists) {
        sprintf( *ppszCommand,
                 "/usr/local/sbin/smbd -b | grep CONFIGFILE");
    }
    else {
        ceError = GPACheckFileExists( "/usr/local/samba/smbd",
                                     &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (bFileExists) {
            sprintf( *ppszCommand,
                     "/usr/local/samba/smbd -b | grep CONFIGFILE");
        } else {
            ceError = GPACheckFileExists( "/usr/local/lib/smb.conf",
                                         &bFileExists);
            BAIL_ON_CENTERIS_ERROR(ceError);

            if (bFileExists) {
                ceError = LwAllocateStringPrintf( ppszFilePath,
                                                  "%s",
                                                  "/usr/local/lib/smb.conf");
                BAIL_ON_CENTERIS_ERROR(ceError);
            } else {
                GPA_LOG_ALWAYS("Could not locate smb.conf system file, hence skipping the setting.");
                return ceError;
            }
        }
    }

error:

    return ceError;
}
#else
static
CENTERROR
CheckSMBDVersion(
    PBOOLEAN pbNewVersion
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszOutput = NULL;
    int i = 0;

    ceError = GPACaptureOutput( "/usr/sbin/smbd -V",
                               &pszOutput);
    if (ceError != CENTERROR_SUCCESS) {
        ceError = CENTERROR_SUCCESS;
        goto error;
    }

    if (pszOutput) {
        while( pszOutput[i++] != ' ');

        if( pszOutput[i++] == '3' ){
            *pbNewVersion = TRUE;
        }
    }

error:

    LW_SAFE_FREE_STRING(pszOutput);

    return ceError;
}

static
CENTERROR
GetSMBConfFilePathOnOthers(
    PSTR *ppszCommand,
    PSTR *ppszFilePath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bFileExists = FALSE;

    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                (PVOID *)ppszCommand);
    BAIL_ON_CENTERIS_ERROR(ceError);

    /* smbd */
    ceError = GPACheckFileExists( "/usr/sbin/smbd",
                                 &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists) {
        BOOLEAN bNewVersion = FALSE;

        ceError = CheckSMBDVersion(&bNewVersion);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if(bNewVersion) {
            sprintf( *ppszCommand,
                     "/usr/sbin/smbd -b | grep CONFIGFILE");
            goto done;
        }
    }

    ceError = GPACheckFileExists( "/etc/samba/smb.conf",
                                 &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists) {
        ceError = LwAllocateStringPrintf( ppszFilePath,
                                          "%s",
                                          "/etc/samba/smb.conf");
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else {
        GPA_LOG_ALWAYS("Could not locate smb.conf system file, hence skipping the setting.");
        return ceError;
    }

done:
error:

    return ceError;
}
#endif
static
CENTERROR
RunCmdAndParseFilePath(
    PSTR pszCommand,
    PSTR *ppszFilePath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    PSTR pszOutput = NULL;
    PSTR pszTmp = NULL;

    ceError = GPACaptureOutput( pszCommand,
                               &pszOutput);
    if (ceError != CENTERROR_SUCCESS) {
        ceError = CENTERROR_SUCCESS;
        goto error;
    }

    if( !pszOutput ) {
        return ceError;
    }

    pszTmp = (char *)strstr( pszOutput,
                             (char *)"/");

    ceError = LwAllocateStringPrintf( ppszFilePath,
                                      "%s",
                                      pszTmp);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if( strstr( *ppszFilePath,
                "\n") != NULL) {
        pszTmp = *ppszFilePath;
        while (*pszTmp++) {
            if (*pszTmp == '\n')
                *pszTmp = '\0';
        }
    }

error:

    LW_SAFE_FREE_STRING(pszOutput);

    return ceError;
}

static
CENTERROR
GetSMBConfFilePath(
    PSTR *ppszFilePath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    PSTR pszCommand = NULL;

#if defined(__LWI_SOLARIS__)
    ceError = GetSMBConfFilePathOnSolaris( &pszCommand,
                                           ppszFilePath);
    BAIL_ON_CENTERIS_ERROR(ceError);
#elif defined(__LWI_AIX__)
    ceError = GetSMBConfFilePathOnAIX( &pszCommand,
                                       ppszFilePath);
    BAIL_ON_CENTERIS_ERROR(ceError);
#else
    ceError = GetSMBConfFilePathOnOthers( &pszCommand,
                                          ppszFilePath);
    BAIL_ON_CENTERIS_ERROR(ceError);
#endif

    if(!IsNullOrEmptyString(*ppszFilePath)) {
        goto done;
    } else if(!IsNullOrEmptyString(pszCommand)) {
        ceError = RunCmdAndParseFilePath( pszCommand,
                                          ppszFilePath);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

done:
error:

    return ceError;
}



#if defined(__LWI_AIX__) || defined (__LWI_HP_UX__)
static
CENTERROR
GetSMBDProcessId(
    pid_t *ppid_smbd,
    FILE *pFile
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszToken = NULL; 
    char szBuf[PATH_MAX+1];
    DWORD iArg = 0;

    while (!*ppid_smbd) {
        if (fgets( szBuf, 
                   PATH_MAX, 
                   pFile) == NULL ) {

            if (feof(pFile))
                break;
            else {
                ceError = LwMapErrnoToLwError(errno);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        }

        LwStripWhitespace(szBuf,1,1);
        pszToken = strtok( szBuf, 
                           " \t");
        iArg = 0;

        while (!IsNullOrEmptyString(pszToken)) {
            if (iArg == 0) {
                if (atoi(pszToken) != 1)
                    break;
            } else if (iArg == 1) {
                *ppid_smbd = atoi(pszToken);
                break;
            }
            pszToken = strtok( NULL, 
                               " \t");
            iArg++;
        } 
    }

error:

    return ceError;
}

static
CENTERROR
SendHUPAIX()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    char szCommand[PATH_MAX + 1];
    FILE *pFile = NULL;
    pid_t pid_smbd = 0;

    /* Find smbd pid */
    memset( szCommand, 
            0, 
            PATH_MAX + 1);

#if defined(__LWI_FREEBSD__)
    sprintf( szCommand,
             "UNIX95=1 ps -U root -o ppid,pid,comm | grep smbd");
#else
    sprintf( szCommand, 
             "UNIX95=1 ps -u root -o ppid,pid,comm | grep smbd");
#endif

    pFile = popen(szCommand, "r");
    if (pFile == NULL)
    {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = GetSMBDProcessId(&pid_smbd, pFile);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (pid_smbd <= 0)
    {
        GPA_LOG_VERBOSE( "smbd is not running ... Hence, no need to send SIGHUP.", 
                         pid_smbd);
    }
    else
    {
        GPA_LOG_VERBOSE( "Sending SIGHUP to smbd of pid: %d", pid_smbd);

        if ( kill( pid_smbd, SIGHUP) < 0 )
        {
            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }

error:

    if (pFile != NULL) {
        pclose(pFile);
    }

    return ceError;

}
#endif

CENTERROR
SendSIGHUP()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    char szCommand[PATH_MAX + 1];

    memset( szCommand, 
            0, 
            PATH_MAX + 1);

    GPA_LOG_VERBOSE("Refreshing samba service ...");

#if defined(__LWI_SOLARIS__)
    sprintf( szCommand, 
             "/usr/bin/pkill -HUP smbd");
#elif defined(__LWI_DARWIN__)
    sprintf( szCommand, 
             "killall -HUP smbd");
#elif defined(__LWI_AIX__) || defined (__LWI_HP_UX__)
    ceError = SendHUPAIX();
    BAIL_ON_CENTERIS_ERROR(ceError);    
#else
    sprintf( szCommand, 
             "killall -HUP -q smbd");
#endif

    ceError = GPARunCommand(szCommand);
    if ( ceError != CENTERROR_SUCCESS ) {
        ceError = CENTERROR_SUCCESS;
        goto error;
    }
    
    return ceError;

error:

    return ceError;
}

CENTERROR
ResetWinbindSettings()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bExists = FALSE;
    PSTR pszFilePath = NULL;
    char szOldFile[STATIC_PATH_BUFFER_SIZE];

    ceError = GPACheckFileExists( LWI_KRB5_CONF_OLD_FILE, 
                                 &bExists );
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    if ( bExists ) {        
        ceError = GPACopyFileWithPerms( LWI_KRB5_CONF_FILE, 
                                       LWIKRB5_BAK_FILEPATH,
                                       S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH );
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = GPACopyFileWithPerms( LWI_KRB5_CONF_OLD_FILE, 
                                       LWI_KRB5_CONF_FILE,
                                       S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH );
        BAIL_ON_CENTERIS_ERROR(ceError);
      
        ceError = LwRemoveFile( LWI_KRB5_CONF_OLD_FILE );
        BAIL_ON_CENTERIS_ERROR(ceError);        
    }

    ceError = GetSMBConfFilePath(&pszFilePath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    sprintf( szOldFile,
             "%s.orig",
             pszFilePath);

    ceError = GPACheckFileExists( szOldFile, 
                                 &bExists );
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    if ( bExists ) {
        ceError = GPACopyFileWithPerms( szOldFile, 
                                       pszFilePath,
                                       S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH );
        BAIL_ON_CENTERIS_ERROR(ceError);
        
        ceError = LwRemoveFile( szOldFile );
        BAIL_ON_CENTERIS_ERROR(ceError);
        
    }

error:

    LW_SAFE_FREE_STRING(pszFilePath);
    return ceError;

}

CENTERROR
ResetCenterisSettings(
    PSTR pszConfFileName, 
    PSTR pszConfFilePath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszBaseFile = NULL;   
    BOOLEAN bExists = FALSE;
    
    ceError = LwAllocateStringPrintf( &pszBaseFile,
                                      "%s/%s", 
                                      "/etc/likewise/", 
                                      pszConfFileName );
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = GPACheckFileExists( pszBaseFile, 
                                 &bExists );
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    if ( bExists ) {        
        ceError = GPACopyFileWithPerms( pszBaseFile, 
                                       pszConfFilePath,
                                       S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH );
        BAIL_ON_CENTERIS_ERROR(ceError);
        
        ceError = LwRemoveFile( pszBaseFile );
        BAIL_ON_CENTERIS_ERROR(ceError);        
    }
   
error:
    
    if (pszBaseFile) {
        LwFreeString( pszBaseFile );
    }

    return ceError;
}

static
CENTERROR
EnsureIncludeLink(
    PCSTR master, 
    PCSTR slave
    )
{
    PCSTR include_comment = "\n"
                            "##\n"
                            "## Include Likewise Enterprise group policy-related auth settings\n"
                            "##\n"; 
    PCSTR include_format = "include = %s\n";
    PSTR include_line = NULL;
    PSTR master_contents = NULL;
    LONG master_size = 0;
    CENTERROR ceError = CENTERROR_SUCCESS;
    char* include_line_found;
    FILE* handle = NULL;
    
    ceError = LwAllocateStringPrintf( &include_line, 
                                      include_format, 
                                      slave);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = GPAReadFile( master, 
                          &master_contents, 
                          &master_size);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    include_line_found = strstr( master_contents, 
                                 include_line);
    
    if (!include_line_found) {
        ceError = GPAOpenFile( master, 
                              "a", 
                              &handle);
        BAIL_ON_CENTERIS_ERROR(ceError);
        
        ceError = GPAFilePrintf( handle, 
                                "%s%s", 
                                include_comment, 
                                include_line);
        BAIL_ON_CENTERIS_ERROR(ceError);
        
        ceError = GPACloseFile(handle);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    LW_SAFE_FREE_STRING(include_line);
    LW_SAFE_FREE_STRING(master_contents);

    return ceError;
}

/* Writes the smb.conf file */
CENTERROR
write_setting(
    PSTR pszName,
    PSTR pszValue,
    PSTR pszType,
    FILE *fp
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszTranslatedValue = NULL;

    if (pszType && strcmp( pszType,
                           LWI_ATTR_TYPE_BOOL) == 0) {

        if (pszValue && strcmp( pszValue,
                                "true") == 0) {
            ceError = LwAllocateString( "yes",
                                        &pszTranslatedValue );
        }
        else {
            ceError = LwAllocateString( "no",
                                        &pszTranslatedValue );
        }
        BAIL_ON_CENTERIS_ERROR(ceError);

        pszValue = pszTranslatedValue;
    }

    ceError = GPAFilePrintf( fp,
                            "    %s = %s\n",
                            pszName,
                            pszValue);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    if ( pszTranslatedValue ) {
        LwFreeString( pszTranslatedValue );
    }

    return ceError;
}


static
CENTERROR
check_write_smb_setting(
    PSTR *ppszLine,
    PSTR *ppszName,
    PSTR *ppszValue,
    FILE *fp,
    PBOOLEAN pbNullPasswd,
    PBOOLEAN pbServerSign,
    PBOOLEAN pbNameCacheTimeOut
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    PSTR pszTranslatedValue = NULL;

    if ( strstr( *ppszLine,
                 LWI_NULL_PASSWD) != NULL &&
         strstr( *ppszName,
                 LWI_NULL_PASSWD) != NULL ) {
        if ( *ppszValue && !strcmp( (char *)*ppszValue,
                                    (char *)"true")) {
            ceError = LwAllocateString( "yes",
                                        &pszTranslatedValue );
        }
        else {
            ceError = LwAllocateString( "no",
                                        &pszTranslatedValue );
        }
        BAIL_ON_CENTERIS_ERROR(ceError);

        *ppszValue = pszTranslatedValue;
        ceError = GPAFilePrintf( fp,
                                "   %s = %s\n",
                                *ppszName,
                                *ppszValue);
        BAIL_ON_CENTERIS_ERROR(ceError);

        *pbNullPasswd = TRUE;
    }
    else if ( strstr( *ppszLine,
                      LWI_SERVER_SIGN) != NULL &&
              strstr( *ppszName,
                      LWI_SERVER_SIGN) != NULL ) {
        ceError = GPAFilePrintf( fp,
                                "   %s = %s\n",
                                *ppszName,
                                *ppszValue);
        BAIL_ON_CENTERIS_ERROR(ceError);
        *pbServerSign = TRUE;
    }
    else if ( strstr( *ppszLine,
                      LWI_NAME_CACHE_TIMEOUT) != NULL &&
              strstr( *ppszName,
                      LWI_NAME_CACHE_TIMEOUT) != NULL ) {
        ceError = GPAFilePrintf( fp,
                                "   %s = %s\n",
                                *ppszName,
                                *ppszValue);
        BAIL_ON_CENTERIS_ERROR(ceError);
        *pbNameCacheTimeOut = TRUE;
    }
    else {
        ceError = GPAFilePrintf( fp,
                                "%s",
                                *ppszLine);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    return ceError;

error:

    return ceError;
}

static
CENTERROR
EditSMBFile(
    FILE *sfp,
    FILE *dfp,
    PAD_SETTING pSMBSetting
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszLine = NULL;

    BOOLEAN bNullPasswdExists = FALSE;
    BOOLEAN bServerSigningExists = FALSE;
    BOOLEAN bNameCacheTimeOut = FALSE;

    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                (PVOID *)&pszLine);
    BAIL_ON_CENTERIS_ERROR(ceError);

    while( fgets( pszLine,
                  STATIC_PATH_BUFFER_SIZE,
                  sfp ) != NULL ) {

        if ( IsComment(pszLine) ){
            ceError = GPAFilePrintf( dfp,
                                    "%s",
                                    pszLine);
            BAIL_ON_CENTERIS_ERROR(ceError);
        } else if ( strstr( pszLine,
                            "[global]") != NULL ) {
            ceError = GPAFilePrintf( dfp,
                                    "%s",
                                    pszLine);
            BAIL_ON_CENTERIS_ERROR(ceError);

            while( fgets( pszLine,
                          STATIC_PATH_BUFFER_SIZE,
                          sfp ) != NULL) {

                if ( IsComment(pszLine) ){
                    ceError = GPAFilePrintf( dfp,
                                            "%s",
                                            pszLine);
                    BAIL_ON_CENTERIS_ERROR(ceError);
                } else if( strstr( pszLine,
                                   "[global]") == NULL &&
                    strstr( pszLine,
                            "[") != NULL) {
                    break;
                } else if ( strstr( pszLine,
                                    ";") != NULL ) {
                    ceError = check_write_smb_setting( &pszLine,
                                                       &pSMBSetting->pszName,
                                                       &pSMBSetting->pszValue,
                                                       dfp,
                                                       &bNullPasswdExists,
                                                       &bServerSigningExists,
                                                       &bNameCacheTimeOut);
                    BAIL_ON_CENTERIS_ERROR(ceError);
                } else {
                    ceError = check_write_smb_setting( &pszLine,
                                                       &pSMBSetting->pszName,
                                                       &pSMBSetting->pszValue,
                                                       dfp,
                                                       &bNullPasswdExists,
                                                       &bServerSigningExists,
                                                       &bNameCacheTimeOut);
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }
                memset( pszLine,
                        0,
                        STATIC_PATH_BUFFER_SIZE);
            }

            if ( !strcmp( (PSTR)pSMBSetting->pszName,
                          (PSTR)LWI_NULL_PASSWD) &&
                 !bNullPasswdExists ) {
                ceError = GPAFilePrintf( dfp,
                                        "   %s = %s\n",
                                        pSMBSetting->pszName,
                                        pSMBSetting->pszValue);
                BAIL_ON_CENTERIS_ERROR(ceError);
            } else if ( !strcmp( (PSTR)pSMBSetting->pszName,
                                 (PSTR)LWI_SERVER_SIGN) &&
                        !bServerSigningExists ) {
                ceError = GPAFilePrintf( dfp,
                                        "   %s = %s\n",
                                        pSMBSetting->pszName,
                                        pSMBSetting->pszValue);
                BAIL_ON_CENTERIS_ERROR(ceError);
           } else if ( !strcmp( (PSTR)pSMBSetting->pszName,
                                (PSTR)LWI_NAME_CACHE_TIMEOUT) &&
                       !bNameCacheTimeOut ) {
                ceError = GPAFilePrintf( dfp,
                                        "   %s = %s\n",
                                        pSMBSetting->pszName,
                                        pSMBSetting->pszValue);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }

            ceError = GPAFilePrintf( dfp,
                                    "%s",
                                    pszLine);
            BAIL_ON_CENTERIS_ERROR(ceError);
        } else {
            ceError = GPAFilePrintf( dfp,
                                    "%s",
                                    pszLine);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        memset( pszLine,
                0,
                STATIC_PATH_BUFFER_SIZE);
    }

error:

    return ceError;
}

CENTERROR
write_smb_conf_setting()
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    PSTR pszLine = NULL;
    PSTR pszFilePath = NULL;
    FILE *sfp = NULL;
    FILE *dfp = NULL;
    char szFilePathGP[STATIC_PATH_BUFFER_SIZE];
    char szOldFile[STATIC_PATH_BUFFER_SIZE];
    char szTmpFile[STATIC_PATH_BUFFER_SIZE];
    BOOLEAN bFileExists = FALSE;
    PAD_SETTING pSMBSetting = NULL;

    if (gpHADSetting) {
        pSMBSetting = gpHADSetting;
    } else {
        goto done;
    }

    ceError = GetSMBConfFilePath(&pszFilePath);

    if( !pszFilePath || ceError != CENTERROR_SUCCESS )
    {
        return CENTERROR_SUCCESS;
    }

    sprintf( szFilePathGP,
             "%s.gp",
             pszFilePath);

    sprintf( szOldFile,
             "%s.orig",
             pszFilePath);

    sprintf( szTmpFile,
             "%s.tmp",
             pszFilePath);

    ceError = GPACheckFileExists( szOldFile,
                                 &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!bFileExists) {
        /* Back up the original smb.conf file so that we are able to revert to it. */
        ceError = GPACheckFileExists( pszFilePath,
                                     &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (bFileExists) {
            GPA_LOG_ALWAYS("Backing up original smb.conf system file.");
            ceError = GPACopyFileWithOriginalPerms( pszFilePath,
                                                   szOldFile);
            BAIL_ON_CENTERIS_ERROR( ceError );
        }
        else{
            GPA_LOG_ALWAYS( "%s system file is not present. Hence, skipping the setting...",
                            pszFilePath);
            return ceError;
        }
    }

    ceError = GPACopyFileWithOriginalPerms( szOldFile,
                                           szFilePathGP);
    BAIL_ON_CENTERIS_ERROR( ceError );

    while(pSMBSetting) {
        ceError = GPAOpenFile( szFilePathGP,
                              "r",
                              &sfp);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = GPAOpenFile( szTmpFile,
                              "w",
                              &dfp);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = EditSMBFile( sfp,
                               dfp,
                               pSMBSetting);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (sfp)
            GPACloseFile(sfp);

        if (dfp)
            GPACloseFile(dfp);

        ceError = GPAMoveFileAcrossDevices( szTmpFile,
                                           szFilePathGP);
        BAIL_ON_CENTERIS_ERROR(ceError);

        pSMBSetting = pSMBSetting->pNext;
    }

    FreeADSetting();

    ceError = GPAMoveFileAcrossDevices( szFilePathGP,
                                       pszFilePath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    LW_SAFE_FREE_STRING(pszLine);
    LW_SAFE_FREE_STRING(pszFilePath);

    return ceError;

done:
error:

    if (sfp)
       GPACloseFile(sfp);

    if (dfp)
       GPACloseFile(dfp);

    LW_SAFE_FREE_STRING(pszLine);
    LW_SAFE_FREE_STRING(pszFilePath);

    return ceError;
}

static
CENTERROR
ParseAndProcessGPItem(
    PGPOLWIGPITEM pNewItem
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    /* Just save out our data directly */
    ceError = GPOLwiWriteItem( LWIAUTHD_WINBIND_POLICY_FILEPATH_GP,
                               NULL,
                               pNewItem);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = write_smb_conf_setting();
    BAIL_ON_CENTERIS_ERROR(ceError);

    /* Copy the staging file to the real one */
    ceError = GPACopyFileWithPerms( LWIAUTHD_WINBIND_POLICY_FILEPATH_GP,
                                   LWIAUTHD_WINBIND_POLICY_FILEPATH,
                                   S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH );
    BAIL_ON_CENTERIS_ERROR(ceError);

    /* Remove the staging file */
    ceError = LwRemoveFile(LWIAUTHD_WINBIND_POLICY_FILEPATH_GP);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}

static
CENTERROR
ProcessWinbindSettingsGPItem(
    PGPOLWIGPITEM pGPItem
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPOLWIGPITEM pNewItem = NULL;

    GPA_LOG_VERBOSE( "Applying new policy to %s",
                     LWIAUTHD_WINBIND_POLICY_FILEPATH);

    /* Make a deep copy of the item to remove the sibling link */
    ceError = GPOCopyGPItem( pGPItem,
                             &pNewItem,
                             TRUE );
    BAIL_ON_CENTERIS_ERROR(ceError);

    /* Just save out our data directly */
    ceError = ParseAndProcessGPItem(pNewItem);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    if (pNewItem) {
        GPODestroyGPItem( pNewItem,
                          TRUE );
    }

    return ceError;
}

static
CENTERROR
ApplyPolicySettings(
    PGPOLWIGPITEM pGPItem,
    BOOLEAN bGPModified
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszBaseFile = NULL;
    BOOLEAN bFileExists = FALSE;
    BOOLEAN bRealExists = FALSE;

    ceError = LwAllocateStringPrintf( &pszBaseFile,
                                      "%s/lwiauthd.policy.conf",
                                      BASEFILESDIR);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPACheckFileExists( pszBaseFile,
                                 &bFileExists );
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPACheckFileExists( LWIAUTHD_WINBIND_POLICY_FILEPATH,
                                 &bRealExists );
    BAIL_ON_CENTERIS_ERROR(ceError);

    if ( bFileExists == FALSE ) {
        /* If the real file exists, save it as our base file */
        if (bRealExists) {
            ceError = GPACopyFileWithOriginalPerms( LWIAUTHD_WINBIND_POLICY_FILEPATH,
                                                   pszBaseFile );
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        /* We have no real file, so we need to write something */
        else {
            bGPModified = TRUE;
        }
    } else if ( !bGPModified ) {
        ceError = ComputeFileTimeStamps( pszBaseFile,
                                         LWIAUTHD_WINBIND_POLICY_FILEPATH,
                                         &bGPModified);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if ( bGPModified ) {
        ceError = ProcessWinbindSettingsGPItem(pGPItem);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    LW_SAFE_FREE_STRING(pszBaseFile);

    return ceError;
}

static
CENTERROR
ApplyWinbindAuthdPolicy(
    PGPOLWIGPITEM pGPItem, 
    BOOLEAN bGPModified
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    ceError = ApplyPolicySettings( pGPItem, 
                                   bGPModified);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = EnsureIncludeLink( LWIAUTHD_FILEPATH, 
                                 LWIAUTHD_WINBIND_POLICY_FILEPATH);
    BAIL_ON_CENTERIS_ERROR(ceError);

    /* Send SIGHUP to SMBD */
    ceError = SendSIGHUP();
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}

CENTERROR
ProcessWinbindSettingsMode(
    PGPOLWIGPITEM pRsopGPAuthItem,
    BOOLEAN bGPModified
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    ceError = ResetCenterisSettings( "lwiauthd.policy.conf",
                                     LWIAUTHD_WINBIND_POLICY_FILEPATH);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = ResetWinbindSettings();
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = ApplyWinbindAuthdPolicy( pRsopGPAuthItem,
                                       bGPModified);
    BAIL_ON_CENTERIS_ERROR(ceError);

    /* Restart the Auth daemon */
    ceError = SignalLwiauthd();
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}

