#include "includes.h"

BOOLEAN
StringStartsWithStr(
    PCSTR pszString,
    PCSTR pszKey
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszCur = NULL, pszFirst = NULL, pszLast = NULL;
    PSTR pszTmp = NULL;
    BOOLEAN bFound = FALSE;
    
    ceError = LwAllocateString( pszString,
                                &pszTmp);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pszCur = pszTmp;
    if ( pszKey )
    {
        LwStripWhitespace(pszCur,1,1);           

        pszFirst = strtok_r(pszCur, " ", &pszLast);

        if ( !strcmp(pszFirst, pszKey) )
            bFound = TRUE;
    }

error:
    LW_SAFE_FREE_STRING(pszTmp);
    return bFound;
}

BOOLEAN
StringStartsWithChar(
    PCSTR pszString,
    CHAR  cKey
    )
{
    PSTR pszCur = (PSTR)pszString;
    BOOLEAN bFound = FALSE;

    if ( pszCur )
    {
        LwStripWhitespace(pszCur,1,1);

        if ( *pszCur == cKey )
        {
            bFound = TRUE;
        }
    }
    return bFound;
}


VOID
StripTabspace(
    PSTR pszString
    )
{
    PSTR pCur = pszString;

    while ( *pCur != '\0' )
    {
        if ( '\t' == *pCur )
        {
            *pCur = ' ';
        }
        pCur++;
    }
}

VOID
GetLastWord(
    PSTR pszString,
    PSTR pszTemp
    )
{
    PSTR pszLast = NULL;
    PSTR pszFirst = NULL;
    PSTR pszNext = NULL;

    LwStripWhitespace(pszString,1,1);

    pszFirst = strtok_r(pszString, " ", &pszNext);

    while ( pszFirst )
    {
        pszLast = pszFirst;
        pszFirst = strtok_r(NULL, " ", &pszNext);
    }

    if ( pszLast )
    {
        strcpy(pszTemp, pszLast);
    }
}

CENTERROR
CreateNewNode(
    PSYSLOGNODE *ppNewNode, 
    PSTR pszKey, 
    PSTR pszValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSYSLOGNODE pNewNode = NULL;
    
    LwStripWhitespace(pszKey,1,1);
    LwStripWhitespace(pszValue,1,1);

    ceError = LwAllocateMemory( sizeof(SYSLOGNODE),
                                (PVOID*)&pNewNode);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    memset(pNewNode, 0 , sizeof(SYSLOGNODE));
    
    ceError = LwAllocateString( pszKey,
                                &pNewNode->pszKey);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateString( pszValue,
                                &pNewNode->pszValue);
    BAIL_ON_CENTERIS_ERROR(ceError);

    *ppNewNode = pNewNode;

    return ceError;

error:
    FreeSyslogList(&pNewNode);
    *ppNewNode = NULL;
    return ceError;
}


VOID
FreeSyslogList(
    PSYSLOGNODE* ppList
    )
{
    PSYSLOGNODE pCur = *ppList;
    PSYSLOGNODE pTemp = NULL;

    while ( pCur ) 
    {
        pTemp = pCur;
        pCur = pCur->pNext;
        
        LW_SAFE_FREE_STRING(pTemp->pszKey);
        LW_SAFE_FREE_STRING(pTemp->pszValue);

        LwFreeMemory(pTemp);
    }

    *ppList = NULL;
}

CENTERROR
AppendNodeToList(
    PSYSLOGNODE *ppMerged,
    PSYSLOGNODE pNewNode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSYSLOGNODE pTemp = *ppMerged;

    if ( pNewNode )
    {   
        if ( !(*ppMerged) )
        {
            *ppMerged = pNewNode;
        }
        else
        {
            while ( pTemp->pNext )
            {
                pTemp = pTemp->pNext;
            }

            pTemp->pNext = pNewNode; 
        }
    }
    return ceError;
}

CENTERROR
CompareAndAppendNode(
    PSYSLOGNODE *ppMerged,
    PSYSLOGNODE pNewNode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSYSLOGNODE pTemp = *ppMerged;
    BOOLEAN bFoundMatch = FALSE;

    if ( pNewNode )
    {   
        if ( !pTemp )
        {
            *ppMerged = pNewNode;
        }
        else
        {
            if ( !strcmp(pTemp->pszKey, pNewNode->pszKey) && 
                 !strcmp(pTemp->pszValue, pNewNode->pszValue) )
            {
                    bFoundMatch = TRUE;
            }
            
            while ( !bFoundMatch && pTemp->pNext )
            {
                pTemp = pTemp->pNext;

                if ( !strcmp(pTemp->pszKey, pNewNode->pszKey) && 
                     !strcmp(pTemp->pszValue, pNewNode->pszValue) )
                {
                        bFoundMatch = TRUE;
                }
            }

            if ( !bFoundMatch ) 
            {
                pTemp->pNext = pNewNode; 
            }
            else 
            {
                FreeSyslogList(&pNewNode);
            }
        }
    }
    
    return ceError;
}

BOOLEAN
IsRsyslogSupported()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bFileExists = FALSE;

    ceError = GPACheckFileExists( LWIRSYSLOG_CONF_FILE, 
                                 &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if ( !bFileExists ) 
    {
        ceError = CENTERROR_INVALID_OPERATION;
        goto error;
    }

error:
    return ceError == CENTERROR_SUCCESS;
}

BOOLEAN
IsSELinuxSupported()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bDirExists = FALSE;

    ceError = GPACheckDirectoryExists( SELINUX_DIRECTORY, 
                                      &bDirExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if ( !bDirExists ) 
    {
        ceError = CENTERROR_INVALID_OPERATION;
        goto error;
    }

error:
    return ceError == CENTERROR_SUCCESS;
}

CENTERROR
AddSELinuxExceptionForSyslog()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    char szCommand[PATH_MAX + 1];

    memset(szCommand, 0, PATH_MAX + 1);

    sprintf(szCommand, "setsebool -P syslogd_disable_trans=1");

    GPA_LOG_VERBOSE("Configuring SELinux to allow syslog to write to system files");
    GPA_LOG_VERBOSE("Running SELinux command: setsebool -P syslogd_disable_trans=1");

    if ( system(szCommand) < 0 ) 
    {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:
    return ceError;
}

BOOLEAN
IsSysLogNG()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bDirExists = FALSE;

    ceError = GPACheckDirectoryExists( LWISYSLOG_NG, 
                                      &bDirExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if ( !bDirExists ) 
    {
        ceError = CENTERROR_INVALID_OPERATION;
        goto error;
    }

error:
    return ceError == CENTERROR_SUCCESS;
}

BOOLEAN
IsApparmorSupported()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bDirExists = FALSE;

    ceError = GPACheckDirectoryExists( APPARMOR_DIR, 
                                      &bDirExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if ( !bDirExists ) 
    {
        ceError = CENTERROR_INVALID_OPERATION;
        goto error;
    }

error:
    return ceError == CENTERROR_SUCCESS;
}

#if defined(__LWI_DARWIN__) || defined(__LWI_FREEBSD__)
CENTERROR
SendHUPToSyslog()
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    ceError = GPARunCommand("killall -HUP syslogd");
    if ( ceError != CENTERROR_SUCCESS ) 
    {
        GPA_LOG_VERBOSE("Could not send HUP to syslog process as it is not running...");
        ceError = CENTERROR_SUCCESS;
    }
    return ceError;
}

#else

static
CENTERROR
GetPID(
    FILE *pFile,
    pid_t *pSyslog_pid
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    pid_t pid_syslog = 0;
    char  szBuf[BUFSIZE+1];
    PSTR pszToken = NULL; 
    DWORD iArg = 0;

    if ( !pFile ) 
    {
        goto error;
    }

    while ( !pid_syslog ) 
    {
        if ( !fgets(szBuf, BUFSIZE, pFile) ) 
        {
            if ( feof(pFile) )
                break;
            else 
            {
                ceError = LwMapErrnoToLwError(errno);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        }

        LwStripWhitespace(szBuf,1,1);

        iArg = 0;
        pszToken = strtok(szBuf, " \t");
        while ( !IsNullOrEmptyString(pszToken) ) 
        {
            if ( iArg == 0 ) 
            {
                if ( atoi(pszToken) != 1 )
                    break;
            } 
            else if ( iArg == 1 ) 
            {
                pid_syslog = atoi(pszToken);
                break;
            }
            pszToken = strtok(NULL, " \t");
            iArg++;
        } 
    }

error:

    *pSyslog_pid = pid_syslog;
    return ceError; 
}

CENTERROR
SendHUPToSyslog()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    char szCommand[PATH_MAX + 1];
    FILE * pFile = NULL;
    pid_t pid_syslog = 0;


    GPA_LOG_VERBOSE("Trying to SIGHUP syslog daemon...");

    memset(szCommand, 0, PATH_MAX + 1);

#if defined(__LWI_DARWIN__)
    sprintf( szCommand,
             "ps -axo ppid,pid,command | grep syslog");
#elif defined(__LWI_FREEBSD__)
    sprintf( szCommand,
             "UNIX95=1 ps -U root -o ppid,pid,comm | grep syslog");
#else
    sprintf( szCommand,
             "UNIX95=1 ps -u root -o ppid,pid,comm | grep syslog");
#endif

    GPA_LOG_VERBOSE("Find Syslog process id running as user root command: %s", szCommand);

    pFile = popen(szCommand, "r");

    //Get PID 
    GetPID(pFile, &pid_syslog);

    if ( pid_syslog <= 0 ) 
    {
        /* Since syslog is not running as user root, now checking whether it is running as user syslog */
        memset(szCommand, 0, PATH_MAX + 1);

#if defined(__LWI_DARWIN__)
        sprintf( szCommand,
                 "ps -axo ppid,pid,command | grep syslog");
#elif defined(__LWI_FREEBSD__)
        sprintf( szCommand,
                 "UNIX95=1 ps -U syslog -o ppid,pid,comm | grep syslog");
#else
        sprintf( szCommand,
                 "UNIX95=1 ps -u syslog -o ppid,pid,comm | grep syslog");
#endif

        GPA_LOG_VERBOSE("Find Syslog process id running as user syslog command: %s", szCommand);

        if ( pFile ) 
        {
            pclose(pFile);
        }

        pFile = popen(szCommand, "r");
        if ( !pFile ) 
        {
            GPA_LOG_VERBOSE("Could not send HUP to syslog process as it is not running...");
            ceError = CENTERROR_SUCCESS;
            goto error;
        }

        GetPID(pFile, &pid_syslog);
        if ( !pid_syslog ) 
        {
            GPA_LOG_VERBOSE("Could not find syslog process id to HUP. Hence ignoring...");
            ceError = CENTERROR_SUCCESS;
            goto error;
        }
    }

    GPA_LOG_VERBOSE("SIGHUP to syslog daemon pid = %d",pid_syslog);
    if ( kill (pid_syslog, SIGHUP) < 0 ) 
    {
        GPA_LOG_ERROR("Unable to SIGHUP to syslog daemon pid = %d",pid_syslog);
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }


error:

    if ( pFile ) 
    {
        pclose(pFile);
    }

    return ceError;
}

#endif //DARWIN OR FREEBSD


static
VOID
ExpandSyslogString(
    PSTR pszFirstStr
    )
{
    CHAR szTempStr[FILEREAD_BUFSIZE] = {0};
    PSTR pszFirst = NULL, pszLast = NULL, pszNext = NULL;
    PSTR pszCommaFirst = NULL, pszCommaNext = NULL;

    strcpy(szTempStr, pszFirstStr);
    memset(pszFirstStr, 0, strlen(pszFirstStr));
    
    pszFirst = strtok_r(szTempStr, ")", &pszLast);
    while ( pszFirst )
    {
        pszNext = strstr(pszFirst, "!(");
        
        if ( pszNext )
        {
            if ( pszNext - pszFirst )
            {
                strncat(pszFirstStr, pszFirst, pszNext - pszFirst);
            }            

            pszNext = pszNext + 2;

            pszCommaFirst = strtok_r(pszNext, ",", &pszCommaNext);

            strcat(pszFirstStr,"!");
            strcat(pszFirstStr, pszCommaFirst);

            pszCommaFirst = strtok_r(NULL, ",", &pszCommaNext);
            while ( pszCommaFirst )
            {
                strcat(pszFirstStr,",!");
                strcat(pszFirstStr, pszCommaFirst);

                pszCommaFirst = strtok_r(NULL, ",", &pszCommaNext);
            }
        }
        else
        {
            if ( pszLast - pszFirst )
            {
                strncat(pszFirstStr, pszFirst, pszLast - pszFirst);
            }
        }
        
        pszFirst = strtok_r(NULL, ")", &pszLast);
    }
}

static 
VOID
SearchString(
    PSTR pszSearchStr,
    PSTR pszSrcStr,
    BOOLEAN* pbDuplicateStr
    )
{
    BOOLEAN bFound = FALSE;
    PSTR pszFirst = NULL, pszLast = NULL;
    CHAR szLocalStr[FILEREAD_BUFSIZE] = {0};

    strcpy(szLocalStr, pszSrcStr);

    pszFirst = strtok_r(szLocalStr, ",", &pszLast);
    while ( pszFirst )
    {   
        if ( !strcmp(pszSearchStr, pszFirst) )
        {
            break;
        }
        pszFirst = strtok_r(NULL, ",", &pszLast);
    }

    if ( pszFirst )
    {
        bFound = TRUE;
    }
    *pbDuplicateStr = bFound;
}

static 
VOID
CompareStrings(
    PSTR pszFirstStr,
    PSTR pszSecondStr,
    BOOLEAN* pbDuplicateStr
    )
{
    BOOLEAN bFound = FALSE;
    PSTR pszSrcFirst = NULL, pszSrcLast = NULL;
    CHAR szFirstStr[FILEREAD_BUFSIZE] = {0};
    CHAR szSecondStr[FILEREAD_BUFSIZE] = {0};

    strcpy(szFirstStr, pszFirstStr);
    strcpy(szSecondStr, pszSecondStr);

    if ( !strcmp(szFirstStr, "*") )
    {
        bFound = TRUE;
    }
    else
    {
        if ( strstr(szFirstStr, "!(") )
        {
           ExpandSyslogString(szFirstStr);
        }

        if ( strstr(szSecondStr, "!(") )
        {
           ExpandSyslogString(szSecondStr);
        }

        pszSrcFirst = strtok_r(szFirstStr, ",",&pszSrcLast);
        while ( pszSrcFirst )
        {
            LwStripWhitespace(pszSrcFirst,1,1);

            SearchString( pszSrcFirst, 
                          szSecondStr, 
                          &bFound);

            if ( !bFound )
            {
                break;
            }
            
            pszSrcFirst = strtok_r(NULL, ",",&pszSrcLast);
        }

        if ( !pszSrcFirst )
        {
            bFound = TRUE;
        }
    }
    *pbDuplicateStr = bFound;
}

static 
VOID
SyslogValueCmp(
    PSTR pszFirstStr,
    PSTR pszSecondStr,
    BOOLEAN* pbDuplicateStr
    )
{
    BOOLEAN bDuplicateStr = FALSE;
    PSTR pszSrcFirst = NULL, pszSrcLast = NULL;
    PSTR pszDstFirst = NULL, pszDstLast = NULL;
    
    pszSrcFirst = strtok_r(pszFirstStr, ".", &pszSrcLast);
    pszDstFirst = strtok_r(pszSecondStr, ".", &pszDstLast);

    while ( pszSrcFirst && pszDstFirst)
    {
        CompareStrings( pszSrcFirst, 
                        pszDstFirst, 
                        &bDuplicateStr);

        if (!bDuplicateStr)
            break;

        CompareStrings( pszDstFirst, 
                        pszSrcFirst, 
                        &bDuplicateStr);

        if (!bDuplicateStr)
            break;

        pszSrcFirst = strtok_r(NULL, ".", &pszSrcLast);
        pszDstFirst = strtok_r(NULL, ".", &pszDstLast);
    }

    if ( !pszSrcFirst && !pszDstFirst )
    {
        bDuplicateStr = TRUE;
    }
   
    *pbDuplicateStr = bDuplicateStr;
}

VOID
IsDuplicateEntry(
    PSYSLOGNODE *ppMasterList, 
    PSTR pszKey,
    PSTR pszValue,
    BOOLEAN* pbDuplicate
    )
{
    PSYSLOGNODE pTemp = *ppMasterList;
    BOOLEAN bDuplicate = FALSE;
    CHAR szFirstKey[FILEREAD_BUFSIZE] = {0};
    CHAR szSecondKey[FILEREAD_BUFSIZE] = {0};
    
    while ( pTemp )
    {
        if ( !strcmp(pTemp->pszValue, pszValue) )
        {
            strcpy(szFirstKey, pTemp->pszKey);
            strcpy(szSecondKey, pszKey);

            SyslogValueCmp( szFirstKey, 
                            szSecondKey, 
                            &bDuplicate);

            if ( bDuplicate )
               break;
        }
        pTemp = pTemp->pNext;
    }
    *pbDuplicate = bDuplicate;
}

VOID
FindClosingSymbol(
    PSTR pszStartPos, 
    PSTR *ppszPos
    )
{
    BOOLEAN bOpenBracket = FALSE; 
    CHAR cChar = '\0';
    int nIndex = -1;

    cChar = pszStartPos[++nIndex];    
    while ( cChar != '\0' )
    {
        if ( cChar == '(' )
        {
            bOpenBracket = TRUE;
        }
        else if ( cChar == ')' )
        {
            if ( bOpenBracket )
            {
                bOpenBracket = FALSE;
            }
            else
            {
                *ppszPos = &pszStartPos[nIndex];
            }
        }
        cChar = pszStartPos[++nIndex];            
    }
}

VOID
StripStartAndEndChar(
    PSTR pszFirst,
    CHAR cStripChar,
    DWORD* pdwLen
    )
{
    int nIndex = 0;
    DWORD dwLen = 0;
    CHAR pszTemp[STATIC_PATH_BUFFER_SIZE] = {0};

    if ( !pdwLen )
    {
        dwLen = strlen(pszFirst);
    }
    else
    {
        dwLen = *pdwLen;
    }

    LwStripWhitespace(pszFirst,1,1);

    if ( pszFirst[0] == cStripChar )
    {
        strncpy(pszTemp, pszFirst+1, dwLen-1);
    }
    else
    {
        strncpy(pszTemp, pszFirst, dwLen);
    }

    memset(pszFirst, 0, strlen(pszFirst));

    nIndex = strlen(pszTemp);

    if ( pszTemp[nIndex - 1] == cStripChar )
    {
        strncpy(pszFirst, pszTemp, nIndex - 1);
        strcat(pszFirst, "\0");
    }
    else
    {
        strcpy(pszFirst, pszTemp);
    }

    if ( pdwLen )
    {
        dwLen = strlen(pszFirst);
        *pdwLen = dwLen;
    }
}


CENTERROR
MergeFileListAndADList(
    PSYSLOGNODE *ppListFromAD,
    PSYSLOGNODE *ppListFromFile
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bDuplicateEntry = FALSE;
    PSYSLOGNODE pTemp = *ppListFromAD;
    PSYSLOGNODE pCur = NULL;

    while ( pTemp )
    {
        pCur = pTemp;
        pTemp = pTemp->pNext;
        pCur->pNext = NULL;

        IsDuplicateEntry( ppListFromFile, 
                          pCur->pszKey,
                          pCur->pszValue,
                          &bDuplicateEntry);

        if ( !bDuplicateEntry )
        {
            ceError = AppendNodeToList( ppListFromFile, 
                                        pCur);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        else
        {
            FreeSyslogList(&pCur);
        }
    }

    *ppListFromAD = *ppListFromFile;

error:
    return ceError;
}

