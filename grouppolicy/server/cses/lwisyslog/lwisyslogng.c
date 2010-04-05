#include "includes.h"

static const PSTR gpszSyslogLevel[] = { "debug", "info", "notice", "warn", "err", "crit", "alert", "emerg" };

static
CENTERROR
GetFilterLoopFromNGFilter(
    PCSTR pszInputStr, 
    PSTR  pszFilterLoop,
    PSTR pszSearchKey
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszFacilityStr = NULL, pszSearchStr = NULL;
    PSTR pszCurPtr = NULL, pszFirst = NULL, pszLast = NULL;
    CHAR szPrevStr[FILEREAD_BUFSIZE] = {0};
    CHAR szCurFacility[FILEREAD_BUFSIZE] = {0};
    CHAR szLastWord[STATIC_PATH_BUFFER_SIZE] = {0};
    BOOLEAN bNotToken = FALSE;
    
    ceError = LwAllocateString( pszInputStr,
                                &pszFacilityStr);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pszCurPtr = pszFacilityStr;
    pszSearchStr = strstr(pszCurPtr, pszSearchKey);

    while ( pszSearchStr )
    {
        if ( pszSearchStr - pszCurPtr )
        {
            strncpy(szPrevStr, pszCurPtr, pszSearchStr - pszCurPtr - 1);

            GetLastWord(szPrevStr, szLastWord);

            LwStripWhitespace(szLastWord,1,1);
        }
        
        if ( pszFilterLoop[0] != '\0' )
        {
            strcat(pszFilterLoop, ",");
        }
        
        if ( *szLastWord )
        {
            if ( !strcmp(szLastWord, "not") )
            {
                bNotToken = TRUE;
            }
            else
            {
                bNotToken = FALSE;
            }
        }

        pszFirst = strchr(pszSearchStr, '(');
        pszLast  = strchr(pszSearchStr, ')'); 

        if ( bNotToken )
        {
            strcpy(szCurFacility,"![");
            strncat(szCurFacility, pszFirst+1, pszLast - pszFirst - 1);
            strcat(szCurFacility, "]");
        }
        else
        {
            strcpy(szCurFacility,"[");
            strncat(szCurFacility, pszFirst+1, pszLast - pszFirst - 1);
            strcat(szCurFacility, "]");
        }

        LwStripWhitespace(szCurFacility,1,1);

        strcat(pszFilterLoop, szCurFacility);

        memset(szLastWord, 0, strlen(szLastWord));
        memset(szPrevStr, 0, strlen(szPrevStr));
        memset(szCurFacility, 0, strlen(szCurFacility));

        pszCurPtr = pszLast;
        pszSearchStr = strstr(pszCurPtr, pszSearchKey);
    }

error:
    LW_SAFE_FREE_STRING(pszFacilityStr);
    return ceError;
}

static
CENTERROR
GetFilterValueFromNGFile(
    PCSTR pszInputStr, 
    PSTR pszFilterValue,
    PSTR pszSearchKey
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bNotToken = FALSE;
    CHAR szPrevStr[FILEREAD_BUFSIZE] = {0};
    CHAR szCurFacility[FILEREAD_BUFSIZE] = {0};
    CHAR szLastWord[STATIC_PATH_BUFFER_SIZE] = {0};
    PSTR pszFacilityStr = NULL, pszSearchStr = NULL;
    PSTR pszCurPtr = NULL, pszFirst = NULL, pszLast = NULL;
    
    ceError = LwAllocateString( pszInputStr,
                                &pszFacilityStr);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pszCurPtr = pszFacilityStr;
    pszSearchStr = strstr(pszCurPtr, pszSearchKey);

    while ( pszSearchStr )
    {
        if ( pszSearchStr - pszCurPtr )
        {
            strncpy(szPrevStr, pszCurPtr, pszSearchStr - pszCurPtr - 1);

            GetLastWord( szPrevStr, szLastWord);

            LwStripWhitespace(szLastWord,1,1);
        }
        
        if ( pszFilterValue[0] != '\0' )
        {
            strcat(pszFilterValue, ",");
        }
        
        if ( *szLastWord )
        {
            if ( !strcmp(szLastWord, "not") )
            {
                bNotToken = TRUE;
            }
            else
            {
                bNotToken = FALSE;
            }
        }

        pszFirst = strchr(pszSearchStr, '(');
        pszLast  = strchr(pszSearchStr, ')'); 

        if ( bNotToken )
        {
            strcpy(szCurFacility,"!(");
            strncat(szCurFacility, pszFirst+1, pszLast - pszFirst - 1);
            strcat(szCurFacility, ")");
        }
        else
        {
            strncpy(szCurFacility, pszFirst+1, pszLast - pszFirst - 1);
            strcat(szCurFacility, "\0");
        }

        LwStripWhitespace(szCurFacility,1,1);

        strcat(pszFilterValue, szCurFacility);

        memset(szLastWord, 0, strlen(szLastWord));
        memset(szPrevStr, 0, strlen(szPrevStr));
        memset(szCurFacility, 0, strlen(szCurFacility));

        pszCurPtr = pszLast;
        pszSearchStr = strstr(pszCurPtr, pszSearchKey);
    }

error:
    LW_SAFE_FREE_STRING(pszFacilityStr);
    return ceError;
}

static
CENTERROR
GetFacilityFromNGFilter(
    PCSTR pszTemp, 
    PSTR pszFilterLoop,
    PSTR pszFilterValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    ceError = GetFilterValueFromNGFile( pszTemp,
                                        pszFilterValue,
                                        "facility");
    BAIL_ON_CENTERIS_ERROR(ceError);

    if ( *pszFilterValue == '\0' && *pszFilterLoop == '\0' )
    {
            pszFilterValue[0] = '*';
    }
    else
    {
        if ( *pszFilterValue != '\0' && *pszFilterLoop != '\0' )
        {
            strcat(pszFilterValue, ",");
        }

        strcat(pszFilterValue, pszFilterLoop);
    }

    strcat(pszFilterValue, ".");

error:
    return ceError;
}

static
CENTERROR
GetMatchFromNGFilter(
    PCSTR pszTemp, 
    PSTR pszMatchValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    ceError = GetFilterValueFromNGFile( pszTemp,
                                        pszMatchValue,
                                        "match");
    BAIL_ON_CENTERIS_ERROR(ceError);
            
error:
    return ceError;
}

static
VOID
AppendMatchValue(
    PSTR pszCurLevel,
    PSTR pszMatch
    )
{
    if ( *pszMatch )
    {
        strcat(pszCurLevel, ".");
        strcat(pszCurLevel, pszMatch);
    }
}

static
VOID
GetDecimalEquivalentOfLevel(
    PSTR pszLevelOne,
    PSTR pszLevelTwo,
    PDWORD pdwIndexOne,
    PDWORD pdwIndexTwo
    )
{
    DWORD dwIndex = 0;
    DWORD dwArraySize = 0;

    dwArraySize = sizeof(gpszSyslogLevel) / sizeof(gpszSyslogLevel[0]);
    
    for ( ; dwIndex < dwArraySize; dwIndex++ )
    {
        if ( !strcmp(pszLevelOne, gpszSyslogLevel[dwIndex]) )
        {
            *pdwIndexOne = dwIndex;
        }

        if ( !strcmp(pszLevelTwo, gpszSyslogLevel[dwIndex]) )
        {
            *pdwIndexTwo = dwIndex;
        }
    }
}

static
VOID
ArrangeLevelInOrder( 
    DWORD dwFirstLevel,
    DWORD dwLastLevel,
    PSTR  pszOperator,
    PSTR  pszLogLevel 
    )
{
    DWORD dwIndex = 0;
    DWORD dwArraySize = 0;

    dwArraySize = sizeof(gpszSyslogLevel) / sizeof(gpszSyslogLevel[0]);

    if ( dwLastLevel == (dwArraySize - 1) )
    {
        strcpy(pszLogLevel, gpszSyslogLevel[dwFirstLevel]);
    }
    else if ( !dwFirstLevel )
    {
        // Level is the first level in the syslog level list
        if ( !strcmp(pszOperator, "not") )
        {
            strcpy(pszOperator, "and");
        }
        else 
        {
            strcpy(pszOperator, "not");
        }
        
        strcpy(pszLogLevel, gpszSyslogLevel[dwLastLevel + 1]);
    }
    else
    {
        for ( dwIndex = dwFirstLevel; dwIndex <= dwLastLevel; dwIndex++ )
        {
            if ( dwIndex != dwFirstLevel )
            {
                strcat(pszLogLevel, ",");
            }

            strcat(pszLogLevel, gpszSyslogLevel[dwIndex]);               
        }
    }
}

static
CENTERROR
LevelOrderCheck(
    PSTR pszInLevel,
    PSTR pszOperator, 
    PSTR pszLevel    
    )
{
    PSTR pszIndex = NULL;
    DWORD dwSkip = 0;
    DWORD dwLastLevel = 0;
    DWORD dwFirstLevel = 0;
    CHAR szLastLevel[8] = {0};
    CHAR szFirstLevel[8] = {0};
    CENTERROR ceError = CENTERROR_SUCCESS;

    if ( ( pszIndex = strstr(pszInLevel, "...") ) )
    {
        dwSkip = 3;
    }
    else if ( ( pszIndex = strstr(pszInLevel, "..") ) )
    {
        dwSkip = 2;
    }
    else if ( ( pszIndex = strstr(pszInLevel, ".") ) )
    {
        dwSkip = 1;
    }

    if ( pszIndex )
    {
        strncpy(szFirstLevel, pszInLevel, (pszIndex - pszInLevel));
        LwStripWhitespace(szFirstLevel, 1, 1);

        strcpy(szLastLevel, pszIndex + dwSkip);
        LwStripWhitespace(szLastLevel, 1, 1);

        GetDecimalEquivalentOfLevel( szFirstLevel, 
                                     szLastLevel,
                                     &dwFirstLevel,
                                     &dwLastLevel);
        
        if ( dwFirstLevel < dwLastLevel )
        {
            ArrangeLevelInOrder( dwFirstLevel,
                                 dwLastLevel,
                                 pszOperator,
                                 pszLevel);
        }
        else if ( dwLastLevel < dwFirstLevel )
        {
            ArrangeLevelInOrder( dwLastLevel,
                                 dwFirstLevel,
                                 pszOperator,
                                 pszLevel);

        }
        else
        {
            strcpy(pszLevel, szFirstLevel);        
        }
    }

    return ceError;
}

static 
VOID 
GetLevelOperator(
    PSTR pszOperator,
    BOOLEAN bEqualLevel,
    PSTR szOperator
    )
{
    if ( *pszOperator && !strcmp(pszOperator, "not"))
    {
        if ( bEqualLevel )
        {
            strcpy(szOperator, "!=");
        }
        else
        {
            strcpy(szOperator, "!");
        }
    }
    else
    {
        if ( bEqualLevel )
        {
            strcpy(szOperator, "=");
        }
        else
        {
            strcpy(szOperator, "");
        }
    }
}

static
CENTERROR
ProcessLevel(
    PSTR pszLevel,
    PSTR pszOperator,
    PSTR pszFacility,
    PSTR pszMatch,
    BOOLEAN bEqualLevel,
    PSTR pszLogLevel
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szOperator[3] = {0};

    memset(pszLogLevel, 0 , strlen(pszLogLevel));

    LwStripWhitespace(pszLevel, 1, 1);

    GetLevelOperator( pszOperator,
                      bEqualLevel,
                      szOperator);

    sprintf(pszLogLevel, "%s%s%s", pszFacility, szOperator, pszLevel);

    AppendMatchValue(pszLogLevel, pszMatch);

    strcat(pszLogLevel, ";");

    return ceError;
}

static
CENTERROR
ProcessNGFilterLevel(
    PSTR pszLev, 
    PSTR pszLastWord, 
    PSTR pszFacility, 
    PSTR pszMatch, 
    PSTR pszCurLevel
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszStart = NULL;
    PSTR pszEnd = NULL;
    CHAR szLev[32] = {0};
    CHAR szLevel[256] = {0};

    if ( strstr(pszLev,".") )  
    {
        LevelOrderCheck( pszLev,
                         pszLastWord,
                         szLev);
        
        if ( strchr(szLev, ',') )
        {
            ceError = ProcessNGFilterLevel( szLev, 
                                            pszLastWord, 
                                            pszFacility, 
                                            pszMatch, 
                                            pszCurLevel);
            BAIL_ON_CENTERIS_ERROR(ceError);

        }
        else
        {
            ceError = ProcessLevel( szLev,
                                    pszLastWord,
                                    pszFacility,
                                    pszMatch,
                                    FALSE,
                                    pszCurLevel);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }
    else if ( strchr(pszLev,',') )
    {
        pszStart = strtok_r(pszLev, ",", &pszEnd);
        while ( pszStart )
        {
            ceError = ProcessLevel( pszStart,
                                    pszLastWord,
                                    pszFacility,
                                    pszMatch,
                                    TRUE,
                                    szLevel);
            BAIL_ON_CENTERIS_ERROR(ceError);

            strcat(pszCurLevel, szLevel);

            pszStart = strtok_r(NULL, ",", &pszEnd);
        }
    }
    else
    {
        ceError = ProcessLevel( pszLev,
                                pszLastWord,
                                pszFacility,
                                pszMatch,
                                TRUE,
                                pszCurLevel);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:
    return ceError;
}

static
CENTERROR
GetLevelFromNGFilter(
    PCSTR pszTemp, 
    PSTR szFacility,
    PSTR szMatch,
    PSTR szFilterValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszLevelStr = NULL, pszFirst = NULL;
    PSTR pszStart = NULL, pszLast = NULL, pszCur = NULL;
    CHAR szLev[FILEREAD_BUFSIZE] = {0};
    CHAR szPrevStr[FILEREAD_BUFSIZE] = {0};
    CHAR szCurLevel[FILEREAD_BUFSIZE] = {0};
    CHAR szOperator[STATIC_PATH_BUFFER_SIZE] = {0};
   
    ceError = LwAllocateString( pszTemp,
                                &pszLevelStr);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pszCur = pszLevelStr;
    pszFirst = strstr(pszCur, "level");

    while ( pszFirst )
    {
        if ( pszFirst - pszCur )
        {
            strncpy(szPrevStr, pszCur, pszFirst - pszCur - 1);

            GetLastWord(szPrevStr, szOperator);

            LwStripWhitespace(szOperator, 1, 1);
        }
        
        pszStart = strchr(pszFirst, '(');
        pszLast = strchr(pszFirst, ')'); 
        strncpy(szLev, pszStart + 1, pszLast - pszStart - 1);

        ceError = ProcessNGFilterLevel( szLev, 
                                        szOperator, 
                                        szFacility, 
                                        szMatch, 
                                        szCurLevel);
        BAIL_ON_CENTERIS_ERROR(ceError);

        LwStripWhitespace(szCurLevel,1,1);

        strcat(szFilterValue, szCurLevel);

        memset(szLev, 0, strlen(szLev));
        memset(szPrevStr, 0, strlen(szPrevStr));
        memset(szCurLevel, 0, strlen(szCurLevel));
        memset(szOperator, 0, strlen(szOperator));

        pszCur = pszLast; 
        pszFirst = strstr(pszCur, "level");
    }

    if ( szFilterValue[0] == '\0' )
    {
        strcpy(szFilterValue, szFacility);
        strcat(szFilterValue,"*");

        AppendMatchValue( szFilterValue, szMatch);
    }

error:
    LW_SAFE_FREE_STRING(pszLevelStr);
    return ceError;
}

BOOLEAN
UpdateApparmorProfile (
    PSTR pszFileName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bFileExists = FALSE;

    //Check if /etc/apparmor.d/sbin.syslog-ng file is present
    ceError = GPACheckFileExists( APPARMOR_NG_FILE,
                                 &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if ( !bFileExists ) 
    {
        //If file name is not present then apparmor wont be restricting 
        //syslog-ng log to any other file, hence just ignore it
        GPA_LOG_VERBOSE("/etc/apparmor.d/sbin.syslog-ng is not present, hence ignoring...");
        ceError = CENTERROR_SUCCESS;
        goto error;
    }
    else 
    {
        //parse and insert file name in the sbin.syslog-ng
        ceError = InsertFileName(pszFileName);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:
    return ceError ;
}

static
CENTERROR
GetKeyValueFromList(
    PSYSLOGNODE pFilterList, 
    PSTR pszFilterKey, 
    PSYSLOGNODE pDestList, 
    PSTR pszDestKey, 
    PSTR pszFilterValue,
    PSTR pszDestValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSYSLOGNODE pTemp = NULL;

    if ( !pszFilterKey || !pFilterList )
    {
        strcpy(pszFilterValue, "*.*");
    }
    else
    {
        pTemp = pFilterList;
        while ( pTemp )
        {
            if ( !strcmp(pTemp->pszKey, pszFilterKey) )
            {
                strcpy(pszFilterValue, pTemp->pszValue);
                break;
            }
            pTemp = pTemp->pNext;
        }
    }
    
    pTemp = pDestList;
    while ( pTemp )
    {
        if ( !strcmp(pTemp->pszKey, pszDestKey) )
        {
            strcpy(pszDestValue, pTemp->pszValue);
            break;
        }
        pTemp = pTemp->pNext;
    }

    return ceError;
}

static
VOID 
GetLogContent( 
    PSTR pszLine,
    PSTR pszSearchKey,
    PSTR pszValue
    )
{
    PSTR pszStr = NULL, pszFirst = NULL, pszLast = NULL;
    int nNameLen = 0;

    pszStr = strstr(pszLine, pszSearchKey);
    if ( pszStr )
    {
        if ( (pszFirst = strstr(pszStr, "(")) )
        {
            if ( (pszLast = strstr(pszFirst, ")")) )
            {
                nNameLen = pszLast - pszFirst - 1;
                memcpy(pszValue, pszFirst+1, nNameLen);
                pszValue[nNameLen] = '\0';
            }
        }
    }
}

static 
CENTERROR
ParseAndGetNGLog(
    PSTR pszLine,
    PSYSLOGNODE* ppFilterList, 
    PSYSLOGNODE* ppDestinationList,
    PSTR pszFilterValue,
    PSTR pszDestValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSYSLOGNODE pFilterList = *ppFilterList;
    PSYSLOGNODE pDestinationList = *ppDestinationList;
    CHAR szDestName[FILEREAD_BUFSIZE] = {0};
    CHAR szFilterName[FILEREAD_BUFSIZE] = {0};

    // find filter list for key
    GetLogContent( pszLine,
                   "filter",
                   szFilterName);

    // find destination list for value
    GetLogContent( pszLine,
                   "destination",
                   szDestName);
    
    ceError = GetKeyValueFromList( pFilterList, 
                                   szFilterName, 
                                   pDestinationList,
                                   szDestName,
                                   pszFilterValue,
                                   pszDestValue);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:
    return ceError;
}

static 
VOID
SetDestValueInOrder(
    PSTR pszDestValue
    )
{
    CHAR szValue[FILEREAD_BUFSIZE] = {0};
    CHAR szPath[FILEREAD_BUFSIZE] = {0};
    CHAR szFSync[FILEREAD_BUFSIZE] = {0};
    CHAR szOwner[32]={0};
    CHAR szGroup[32] = {0};
    CHAR szPerms[32] = {0};
    PSTR pszFirst = NULL;
    PSTR pszLast = NULL;

    strcpy(szValue, pszDestValue);

    LwStripWhitespace(szValue,1,1); 
    
    memset(pszDestValue, 0 , strlen(pszDestValue));
   
    pszFirst = strtok_r(szValue, " ", &pszLast);
    strcpy(szPath, pszFirst);

    LwStripWhitespace(szPath,1,1); 

    pszFirst = strtok_r(NULL, " ", &pszLast);
    while ( pszFirst )
    {
        if ( strstr(pszFirst, "fsync") )
        {
            strcpy(szFSync, pszFirst);
            LwStripWhitespace(szFSync,1,1); 
        }
        else if ( strstr(pszFirst, "owner") )
        {
            strcpy(szOwner, pszFirst);
            LwStripWhitespace(szOwner,1,1); 
        }
        else if ( strstr(pszFirst, "group") )
        {
            strcpy(szGroup, pszFirst);
            LwStripWhitespace(szGroup,1,1); 
        }
        else if ( strstr(pszFirst, "perm") )
        {
            strcpy(szPerms, pszFirst);
            LwStripWhitespace(szPerms,1,1); 
        }
                    
        pszFirst = strtok_r(NULL, " ", &pszLast);
    }

    strcpy(pszDestValue, szPath);
    if ( *szFSync )
    {
        strcat(pszDestValue, " ");
        strcat(pszDestValue, szFSync);
    }

    if ( *szOwner )
    {
        strcat(pszDestValue, " ");
        strcat(pszDestValue, szOwner);
    }
    
    if ( *szGroup )
    {
        strcat(pszDestValue, " ");
        strcat(pszDestValue, szGroup);
    }

    if ( *szPerms )
    {
        strcat(pszDestValue, " ");
        strcat(pszDestValue, szPerms);
    }
}

static
VOID
GetDestinationValue(
    PSTR pszTemp,
    PSTR pszDestValue
    )
{
    PSTR pszCur = NULL;
    PSTR pszFirst = NULL;
    PSTR pszLastStr = NULL;
    int nIndex = 0;

    if ( (pszCur = strstr(pszTemp, "file")) )
    {
        pszDestValue[nIndex++] = '-';
    }
    else if ( (pszCur = strstr(pszTemp, "pipe")) )
    {
        pszDestValue[nIndex++] = '|';
    }
    else if ( (pszCur = strstr(pszTemp, "tcp")) )
    {
        pszDestValue[nIndex++] = '@';
    }
    else if ( (pszCur = strstr(pszTemp, "usertty")) )
    {
        pszDestValue[nIndex++] = '*';
    }

    if ( *pszDestValue )
    {
        pszFirst = strchr(pszCur, '(');
        FindClosingSymbol(pszFirst+1, &pszLastStr);
    }

    if ( pszFirst && pszLastStr )
    {
        if ( pszLastStr - pszFirst )
        {
            strncpy(&pszDestValue[nIndex], pszFirst + 1, pszLastStr - pszFirst - 1);
            pszDestValue[pszLastStr - pszFirst] = '\0';

            SetDestValueInOrder(pszDestValue);
        }
    }

    LwStripWhitespace(pszDestValue,1,1);
}

static 
CENTERROR
ParseAndGetNGDestination(
    PSTR pszLine,
    PSYSLOGNODE *ppNewNode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSYSLOGNODE pNewNode = NULL;
    PSTR pszFirst = NULL, pszDestName = NULL;
    PSTR pszLast = NULL, pszStr = NULL, pszTemp = NULL; 
    CHAR szDestValue[FILEREAD_BUFSIZE] = {0};
                
    pszFirst = strtok_r(pszLine,"{",&pszLast);
    pszStr = strtok_r(pszFirst," ",&pszDestName);

    LwStripWhitespace(pszDestName,1,1);

    if ( pszLast )
    {
        pszTemp = strtok_r(pszLast, "}", &pszStr);

        GetDestinationValue(pszTemp, szDestValue);
    }
    
    if ( pszDestName && *szDestValue )
    {
        ceError = CreateNewNode( &pNewNode, 
                                 pszDestName, 
                                 szDestValue);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    *ppNewNode = pNewNode;
    return ceError;

error:
    *ppNewNode = NULL;
    return ceError;
}

static
CENTERROR
GetFilterValue(
    PSTR pszTemp,
    PSTR pszFilterValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szTemp[FILEREAD_BUFSIZE] = {0};
    CHAR szMatch[FILEREAD_BUFSIZE] = {0};
    CHAR szFacility[FILEREAD_BUFSIZE] = {0};
    CHAR szFilterLoop[FILEREAD_BUFSIZE] = {0};
    PSTR pszStart = NULL, pszEnd = NULL;

    pszStart = pszTemp;
    while ( pszStart )
    {
        pszEnd = strstr(pszStart, " or ");
        if ( pszEnd )
        {
            strncpy(szTemp, pszStart, pszEnd - pszStart);
            strcat(szTemp, "\0");

            LwStripWhitespace(pszEnd,1,1);

            pszStart = pszEnd + 2;

            LwStripWhitespace(pszStart,1,1);

            pszEnd = strstr(pszStart, " or ");
        }
        else
        {
            strcpy(szTemp, pszStart);
            pszStart = NULL;
        }

        ceError = GetFilterLoopFromNGFilter( szTemp, 
                                             szFilterLoop,
                                             "filter");
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = GetFacilityFromNGFilter( szTemp,
                                           szFilterLoop,
                                           szFacility);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = GetMatchFromNGFilter( szTemp, 
                                        szMatch);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = GetLevelFromNGFilter( szTemp,
                                        szFacility,
                                        szMatch,
                                        pszFilterValue);
        BAIL_ON_CENTERIS_ERROR(ceError);

        memset(szTemp, 0, strlen(szTemp));
        memset(szMatch, 0, strlen(szMatch));
        memset(szFacility, 0, strlen(szFacility));
        memset(szFilterLoop, 0, strlen(szFilterLoop));
    }

error:
    return ceError;
}

static
VOID
GetFilterValueFromList(
    PSYSLOGNODE *ppMerged,
    PSTR pszFilterName,
    PSTR pszFilterValue
    )
{
    PSYSLOGNODE pTemp = *ppMerged;

    while ( pTemp )
    {
        if ( !strcmp(pTemp->pszKey, pszFilterName) )
        {
            strcpy(pszFilterValue, pTemp->pszValue);
            break;
        }
        pTemp = pTemp->pNext;
    }
}

static
CENTERROR
ExpandFilterInFacility(
    PSTR pszFilter,
    PSYSLOGNODE *ppMerged,
    PSTR pszFilterValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szFilterName[FILEREAD_BUFSIZE] = {0};
    CHAR szFilterValue[FILEREAD_BUFSIZE] = {0};
    PSTR pszFirst = NULL, pszLast = NULL;
    PSTR pszNext = NULL, pColon = NULL;
    PSTR pszStart = NULL;
    DWORD dwPos = 0;
    
    ceError = LwAllocateString( pszFilter,
                                &pszStart);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pszFirst = strtok_r(pszStart, "]", &pszLast);
    while ( pszFirst )
    {
        if ( ( pszNext = strchr(pszFirst, '[') ) )
        {
            dwPos = pszNext - pszFirst;

            pszNext = pszNext + 1;

            strncpy(szFilterName, pszNext, pszLast - pszNext);

            GetFilterValueFromList( ppMerged,
                                    szFilterName,
                                    szFilterValue);
            if ( *szFilterValue )
            {
                strncat(pszFilterValue, pszFirst, dwPos);
                strcat(pszFilterValue,"[");

                pColon = strchr(szFilterValue, ';');
                while ( pColon )
                {
                    *pColon = '+';
                    pColon = strchr(pColon+1, ';');
                }

                strcat(pszFilterValue, szFilterValue);
                strcat(pszFilterValue,"]");
            }
            else if ( dwPos ) //If unknown filtername copy value prior to filter name 
            {
                if ( ( strstr(pszFirst, "![" ) ) ) 
                {
                    strncat(pszFilterValue, pszFirst, dwPos - 2); //Skips both , and ! characters
                }
                else
                {
                    strncat(pszFilterValue, pszFirst, dwPos - 1); //Skips , from the string
                }
            }
        }
        else if ( pszLast - pszFirst )
        {
                strncat(pszFilterValue, pszFirst, pszLast - pszFirst);
        }
        
        pszFirst = strtok_r(NULL, "]", &pszLast);
    }
   
error:
    LW_SAFE_FREE_STRING(pszStart);
    return ceError;
}

static
CENTERROR
ExpandFilterInFilter(
    PSTR pszFilter,
    PSYSLOGNODE *ppMerged,
    PSTR pszFilterValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszLevel = NULL;
    PSTR pszStart = NULL;
    PSTR pszFacility = NULL;
    
    ceError = LwAllocateString( pszFilter,
                                &pszStart);
    BAIL_ON_CENTERIS_ERROR(ceError);

    // Level and match may contains character '[' and ']', So 
    // skipping the level and match and getting the facility
    pszFacility = strtok_r(pszStart, ".", &pszLevel); 

    ceError = ExpandFilterInFacility( pszFacility,
                                      ppMerged,
                                      pszFilterValue);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    if ( pszLevel && *pszLevel)
    {
        strcat(pszFilterValue,".");
        strcat(pszFilterValue, pszLevel);
    }
         
error:
    LW_SAFE_FREE_STRING(pszStart);
    return ceError;
}

static
CENTERROR
ProcessFilterInFilter(
    PSTR pszInFilter,
    PSYSLOGNODE *ppMerged,
    PSTR pszFilterValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszLast = NULL;
    PSTR pszFirst = NULL;
    PSTR pszFilter = NULL;
    
    ceError = LwAllocateString( pszInFilter,
                                &pszFilter);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pszFirst = strtok_r(pszFilter, ";", &pszLast);
    while ( pszFirst )
    {
        if ( *pszFilterValue )
        {
            strcat(pszFilterValue, ";");
        }

        ceError = ExpandFilterInFilter( pszFirst,
                                        ppMerged,
                                        pszFilterValue);
        BAIL_ON_CENTERIS_ERROR(ceError);
         
        pszFirst = strtok_r(NULL, ";", &pszLast);
    }
    
error:
    LW_SAFE_FREE_STRING(pszFilter);
    return ceError;
}
    
static
CENTERROR
ParseAndGetNGFilter(
    PSTR pszLine,
    PSYSLOGNODE *ppMerged,
    PSYSLOGNODE *ppNode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszFirst = NULL, pszLast = NULL, pszStr = NULL;
    PSTR pszFilterName = NULL, pszTemp = NULL;
    CHAR szFilter[FILEREAD_BUFSIZE] = {0};
    CHAR szFilterValue[FILEREAD_BUFSIZE] = {0};
    PSYSLOGNODE pNew = NULL;

    pszFirst = strtok_r(pszLine,"{",&pszLast);
    pszStr = strtok_r(pszFirst," ",&pszFilterName);

    LwStripWhitespace(pszFilterName,1,1);

    if ( pszLast )
    {
        pszTemp = strtok_r(pszLast, "}", &pszStr);

        ceError = GetFilterValue( pszTemp, 
                                  szFilter);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = ProcessFilterInFilter( szFilter,
                                     ppMerged,
                                     szFilterValue);
    BAIL_ON_CENTERIS_ERROR(ceError);
   
    ceError = CreateNewNode( &pNew, 
                             pszFilterName, 
                             szFilterValue);
    BAIL_ON_CENTERIS_ERROR(ceError);

    *ppNode = pNew;
    return ceError;

error:
    *ppNode = NULL;
    return ceError;
}


CENTERROR
AppendNGEntryToList(
    PSTR pszKey, 
    PSTR pszValue,
    PSYSLOGNODE* ppMasterList 
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bDuplicateEntry = FALSE;
    BOOLEAN bActive = TRUE;
    PSYSLOGNODE pNew = NULL;
    PSTR pszFirst = NULL, pszLast = NULL;
    CHAR szKey[FILEREAD_BUFSIZE] = {0};

    if ( strchr(pszKey, '#') )
    {
        bActive = FALSE;
    }

    pszFirst = strtok_r(pszKey, ";", &pszLast);
    while ( pszFirst )
    {
        IsDuplicateEntry( ppMasterList, 
                          pszFirst,
                          pszValue,
                          &bDuplicateEntry);

        if ( bDuplicateEntry )
        {
            pszFirst = strtok_r(NULL, ";", &pszLast);
            continue;
        }

        // If ng entry is inactive append '#' 
        if ( FALSE == bActive )
        {
            if ( !strchr(pszFirst,'#') )
            {
                strcpy(szKey, "#");
            }
        }
        strcat(szKey, pszFirst);

        ceError = CreateNewNode( &pNew,
                                 szKey,
                                 pszValue);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = AppendNodeToList( ppMasterList, 
                                    pNew);
        BAIL_ON_CENTERIS_ERROR(ceError);

        memset(szKey, 0, strlen(szKey));

        pszFirst = strtok_r(NULL, ";", &pszLast);
    }

error:
    return ceError;
}

static 
CENTERROR
ProcessNGFileEntry(
    PSTR szTempLine, 
    PSYSLOGNODE *ppMerged, 
    PSYSLOGNODE *ppFilterList, 
    PSYSLOGNODE *ppDestinationList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szDestValue[FILEREAD_BUFSIZE] = {0};
    CHAR szFilterValue[FILEREAD_BUFSIZE] = {0};
    PSYSLOGNODE pNewNode = NULL;

    if ( StringStartsWithStr(szTempLine, "log") )
    {
        ceError = ParseAndGetNGLog( szTempLine, 
                                    ppFilterList, 
                                    ppDestinationList,
                                    szFilterValue,
                                    szDestValue);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = AppendNGEntryToList( szFilterValue,
                                       szDestValue,
                                       ppMerged);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    else if ( StringStartsWithStr(szTempLine, "destination") )
    {
        ceError = ParseAndGetNGDestination( szTempLine, 
                                            &pNewNode);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = CompareAndAppendNode( ppDestinationList, 
                                        pNewNode);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    else if ( StringStartsWithStr(szTempLine, "filter") )
    {
        ceError = ParseAndGetNGFilter( szTempLine,
                                       ppFilterList,
                                       &pNewNode);
        BAIL_ON_CENTERIS_ERROR(ceError);
    
        ceError = CompareAndAppendNode( ppFilterList, 
                                        pNewNode);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:
    return ceError;
}

CENTERROR
PrepareListFromNGFile(
    PCSTR pszOrigFile,
    PSYSLOGNODE *ppMerged
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szLine[FILEREAD_BUFSIZE] = {0};
    CHAR szTempLine[FILEREAD_BUFSIZE] = {0};
    PSYSLOGNODE pFilterList = NULL;
    PSYSLOGNODE pDestinationList = NULL;
    BOOLEAN bAppendLine = FALSE;
    FILE* fpFile = NULL;
    
    ceError = GPAOpenFile( pszOrigFile,
                          "r",
                          &fpFile);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    if ( ceError != CENTERROR_SUCCESS )
    {
        GPA_LOG_VERBOSE( "Failed to open the syslog-ng file from path [%s]",
                         pszOrigFile);
        ceError = CENTERROR_SUCCESS;
        goto error;
    }

    while ( fgets(szLine, FILEREAD_BUFSIZE, fpFile) ) 
    {
        StripTabspace(szLine);

        LwStripWhitespace(szLine,1,1);        

        if ( strlen(szLine) && !StringStartsWithChar(szLine,'#') ) 
        {
            if ( strchr(szLine, '}') )
            {
                if ( bAppendLine )
                {
                    strcat(szTempLine, szLine);
                    bAppendLine = FALSE;
                }
                else
                {
                    strcpy(szTempLine, szLine);
                }
                
                ceError = ProcessNGFileEntry( szTempLine, 
                                              ppMerged, 
                                              &pFilterList, 
                                              &pDestinationList);
                BAIL_ON_CENTERIS_ERROR(ceError);

                memset(szTempLine, 0, strlen(szTempLine));
            }
            else
            {
                bAppendLine = TRUE;
                strcat(szTempLine, szLine);
                strcat(szTempLine, " ");
            }
        }
    }

error:

    if ( fpFile )
    {
        GPACloseFile(fpFile);
        fpFile = NULL;
    }

    FreeSyslogList(&pDestinationList);
    FreeSyslogList(&pFilterList);

    return ceError;
}
