#include "includes.h"

BOOLEAN
InsertFileName (
    PSTR pszFileName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    FILE *fp = NULL;
    FILE *fpgp = NULL;
    PSTR pszLine = NULL, pszTmpLine = NULL;
    PSTR pszLast = NULL, pszToken = NULL;
    BOOLEAN bSetTab = FALSE;
    BOOLEAN bMatchNotFound = TRUE;
    CHAR szSetFileName[STATIC_PATH_BUFFER_SIZE] = {0};

    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                (PVOID*)&pszLine);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                (PVOID*)&pszTmpLine);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAOpenFile( APPARMOR_NG_FILE,
                          "r",
                          &fp);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAOpenFile( APPARMOR_NG_FILE_GP,
                          "w",
                          &fpgp);
    BAIL_ON_CENTERIS_ERROR(ceError);

    while( fgets(pszLine, STATIC_PATH_BUFFER_SIZE, fp)) 
    {
        LwStripWhitespace(pszLine,1,1);

        if ( strcspn(pszLine,"#}") != 0 ) 
        {
            if ( strchr(pszLine,'{') ) 
            {
                ceError = GPAFilePrintf( fpgp,
                                        "%s\n",
                                        pszLine);
                BAIL_ON_CENTERIS_ERROR(ceError);

                bSetTab = TRUE;
            }
            else 
            {
                strcpy(pszTmpLine,pszLine);

                pszToken = strtok_r(pszTmpLine," ",&pszLast);

                if ( strcmp(pszToken,pszFileName) && bMatchNotFound ) 
                    bMatchNotFound = TRUE; 
                else 
                    bMatchNotFound = FALSE; 

                ceError = GPAFilePrintf( fpgp,
                                        "  %s\n",
                                        pszLine);
                BAIL_ON_CENTERIS_ERROR(ceError);

            }
        }
        else if ( strchr(pszLine,'}') ) 
        {
            if ( bMatchNotFound ) 
            {
                //insert file name with write permission 
                sprintf( szSetFileName,
                         "%s w,",
                         pszFileName);

                ceError = GPAFilePrintf( fpgp,
                                        "  %s\n}\n",
                                        szSetFileName);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
            else 
            {
                ceError = GPAFilePrintf( fpgp,
                                        "%s\n",
                                        pszLine);
                BAIL_ON_CENTERIS_ERROR(ceError);

            }
        }
        else 
        {
            if( bSetTab ) 
            {
                ceError = GPAFilePrintf( fpgp,
                                        "  %s\n",
                                        pszLine);
                BAIL_ON_CENTERIS_ERROR(ceError);
            } 
            else 
            {
                ceError = GPAFilePrintf( fpgp,
                                        "%s\n",
                                        pszLine);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        }
    }

    if ( fpgp ) 
    {
        fclose(fpgp);
    }

    //Move the file
    ceError = GPAMoveFileAcrossDevices( APPARMOR_NG_FILE_GP,
                                       APPARMOR_NG_FILE );
    BAIL_ON_CENTERIS_ERROR(ceError);

error:
    if ( fp ) 
    {
        fclose(fp);
    }

    LW_SAFE_FREE_STRING(pszTmpLine);
    LW_SAFE_FREE_STRING(pszLine);

    return ceError;
}

void 
WriteNGOptions( 
    FILE *sngfp
    )
{
    fprintf( sngfp, 
             "%s\n",
             LWISYSLOG_NG_OPTIONS);
} 

void 
WriteNGDefSrc(
    FILE *sngfp
    )
{
    fprintf( sngfp, 
             "%s\n\t%s\n\t%s\n\t%s\n%s\n\n",
             "source src {",
             "internal();",
             "unix-stream(\"/dev/log\");",
             "unix-dgram(\"/dev/log\");",
             "};");
} 

static
void 
WriteNGDest( 
    FILE *sngfp,
    PSTR filepath,
    PSTR destName,
    BOOLEAN bActiveLog
    )
{
    CHAR szDest[STATIC_PATH_BUFFER_SIZE] = {0};

    if (!bActiveLog)
    {
        strcpy(szDest,"#destination");
    }
    else
    {
        strcpy(szDest,"destination");
    }

   if ( filepath )
   {
       if ( strspn(filepath, "-")) 
       {
            fprintf( sngfp, 
                     "%s %s { file(%s) ; };\n",
                     szDest,
                     destName,
                     filepath+1);

       }
       else if ( strspn(filepath, "|")) 
       {
            fprintf( sngfp, 
                     "%s %s { pipe(%s); };\n",
                     szDest,
                     destName,
                     filepath+1);
       }
       else if ( strspn(filepath, "@")) 
       {
            fprintf( sngfp, 
                     "%s %s { tcp(%s port(514)); };\n",
                     szDest,
                     destName,
                     filepath+1);

       }
       else if ( strspn(filepath, "*")) 
       {
            fprintf( sngfp, 
                     "%s %s { usertty(%s); };\n",
                     szDest,
                     destName,
                     filepath+1);
       }
       else if ( strspn(filepath, "/")) 
       {
            fprintf( sngfp, 
                     "%s %s { file(%s); };\n",
                     szDest,
                     destName,
                     filepath);
       }
       else if ( strcmp(filepath, "") )
       {
            fprintf( sngfp, 
                     "%s %s { usertty(%s); };\n",
                     szDest,
                     destName,
                     filepath);
       }
       else
       {
            fprintf( sngfp, 
                     "%s %s { };\n",
                     szDest,
                     destName);
       }
   }
} 

static
CENTERROR
ParseMatchString(
    PSTR pszMatchStr,
    PSTR pszOperator,
    PSTR pszFormatStr,
    PSTR pszFormat
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szAppendSymbol[2] = {0};
    PSTR pszMatch = NULL;

    ceError = LwAllocateString( pszMatchStr,
                                &pszMatch);
    BAIL_ON_CENTERIS_ERROR(ceError);

    memset(pszFormat, 0, strlen(pszFormat));

    if ( StringStartsWithChar(pszMatch, '"') )
    {
        StripStartAndEndChar(pszMatch, '"', NULL);
        strcpy(szAppendSymbol, "\"");
    }
    else if ( StringStartsWithChar(pszMatch, '\'') )
    {
        StripStartAndEndChar(pszMatch, '\'', NULL);
        strcpy(szAppendSymbol, "'");
    }
    else
    {
        strcpy(szAppendSymbol, "'");
    }
   
    sprintf( pszFormat, 
             "%s%s%s%s%s%s%s", 
             pszOperator,
             pszFormatStr,
             "(",
             szAppendSymbol,
             pszMatch,
             szAppendSymbol,
             ")" );

error:
   LW_SAFE_FREE_STRING(pszMatch);
   return ceError;
}

static
CENTERROR
FormatMatchString(
    PSTR pszMatchStr,
    PSTR pszFormatStr,
    PSTR pszOperator,
    PSTR pszMatchString
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszEnd = NULL;
    PSTR pszMatch = NULL;
    PSTR pszStart = NULL;
    CHAR szFormat[STATIC_PATH_BUFFER_SIZE] = {0};

    ceError = LwAllocateString( pszMatchStr,
                                &pszMatch);
    BAIL_ON_CENTERIS_ERROR(ceError);

    memset(pszMatchString, 0, strlen(pszMatchString));

    pszStart = strtok_r(pszMatch, ",", &pszEnd);
    while ( pszStart )
    {
        ceError = ParseMatchString( pszStart,
                                    pszOperator,
                                    pszFormatStr,
                                    szFormat );
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcat(pszMatchString, szFormat);

        pszStart = strtok_r(NULL, ",", &pszEnd);
    }                            

error:
    LW_SAFE_FREE_STRING(pszMatch);
    return ceError;
}

static 
CENTERROR
MergeMatchString(
    PSTR pszFirst,
    PSTR pszNext,
    PSTR pszFormatStr,
    PSTR pszMatchString
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    DWORD dwPos = 0;
    CHAR szResMatch[FILEREAD_BUFSIZE] = {0};
    CHAR szMatch[STATIC_PATH_BUFFER_SIZE] = {0};
    
    dwPos = pszNext - pszFirst;

    if ( dwPos )
    {
        strncpy(szMatch, pszFirst, dwPos);
        StripStartAndEndChar(szMatch, ',', &dwPos);

        ceError = FormatMatchString( szMatch,
                                     pszFormatStr,
                                     " and ",
                                     szResMatch );
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcat(pszMatchString, szResMatch);
    } 

error:
    return ceError;
}

static
CENTERROR
ProcessMatchList(
    PSTR pszMatchStr,
    PSTR pszFormatStr,
    PSTR pszMatchString
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszNext = NULL;
    PSTR pszLast = NULL;
    PSTR pszFirst = NULL;
    PSTR pszMatch = NULL;
    CHAR szMatch[FILEREAD_BUFSIZE] = {0};
    CHAR szFac[STATIC_PATH_BUFFER_SIZE] = {0};

    ceError = LwAllocateString( pszMatchStr,
                                &pszMatch);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    pszFirst = strtok_r(pszMatch, ")", &pszLast);
    while ( pszFirst )
    {
        //Match may come like M1, M2, !(M3), M4
        pszNext = strstr(pszFirst, "!(");
        if ( pszNext )
        {
            ceError = MergeMatchString( pszFirst,
                                        pszNext,
                                        pszFormatStr,
                                        pszMatchString );
            BAIL_ON_CENTERIS_ERROR(ceError);

            strcpy(szFac, pszNext + 2);
            
            ceError = FormatMatchString( szFac,
                                         pszFormatStr,
                                         " and not ",
                                         szMatch );
            BAIL_ON_CENTERIS_ERROR(ceError);

            strcat(pszMatchString, szMatch);
        }
        else
        {
            ceError = MergeMatchString( pszFirst,
                                        pszLast,
                                        pszFormatStr,
                                        pszMatchString );
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        pszFirst = strtok_r(NULL, ")", &pszLast);
    }
    
error:
    LW_SAFE_FREE_STRING(pszMatch);
    return ceError;
}

static
CENTERROR
FormatNGFilterMatchString(
    PSTR pszMatchStr,
    PSTR pszFormatStr,
    PSTR *ppszMatchString
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszMatch = NULL;
    PSTR pszMatchString = NULL;
    CHAR szMatchString[FILEREAD_BUFSIZE] = {0};

    ceError = LwAllocateString( pszMatchStr,
                                &pszMatch);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if ( *pszMatch )
    {
        if ( strchr(pszMatch, '!') )
        {
            ceError = ProcessMatchList( pszMatch,
                                        pszFormatStr,
                                        szMatchString );
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        else
        {
            ceError = FormatMatchString( pszMatch,
                                         pszFormatStr,
                                         " and ",
                                         szMatchString );
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        ceError = LwAllocateString( szMatchString,
                                    &pszMatchString);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

cleanup:
    LW_SAFE_FREE_STRING(pszMatch);
    *ppszMatchString = pszMatchString;
    return ceError;

error:
    LW_SAFE_FREE_STRING(pszMatchString);
    pszMatchString = NULL;
    goto cleanup;
}

static
VOID
GetFacilityAndFilter(
    PSTR pszFacilities, 
    PSTR szFacilityLst, 
    PSTR szFilterLst)
{
    PSTR pszFirst = NULL, pszNext = NULL;
    DWORD dwFacilityLen = 0;

    if ( pszFacilities )
    {
        pszFirst = strstr(pszFacilities,"![");
        pszNext = strchr(pszFacilities, '[');

        if ( pszFirst > pszNext )
        {
            dwFacilityLen = pszNext - pszFacilities;
            strncpy(szFacilityLst, pszFacilities, dwFacilityLen);
            strcpy(szFilterLst, pszNext);        
        }
        else if ( pszFirst < pszNext )
        {
            dwFacilityLen = pszFirst - pszFacilities;
            strncpy(szFacilityLst, pszFacilities, dwFacilityLen);
            strcpy(szFilterLst, pszFirst);        
        }
        else
        {
            strcpy(szFacilityLst, pszFacilities);
        }
    }
}

static
CENTERROR
FormatNGFilterString(
    PSTR pszFacilities,
    PSTR *ppszFacilityStr,
    PSTR pszFormatStr
    )
{

    CENTERROR ceError = 0;
    DWORD dwPos = 0;
    BOOLEAN bNotToken = FALSE;
    CHAR szFilterLst[FILEREAD_BUFSIZE] = {0};
    CHAR szFac[STATIC_PATH_BUFFER_SIZE] = {0};
    CHAR szFacilityLst[FILEREAD_BUFSIZE] = {0};
    CHAR szFacilityString[FILEREAD_BUFSIZE] = {0};
    PSTR pszFirst = NULL, pszNext = NULL, pszLast = NULL;
    PSTR pszFacilityString = NULL, pszStart = NULL, pszEnd = NULL;

    GetFacilityAndFilter( pszFacilities, 
                          szFacilityLst, 
                          szFilterLst);

    if ( *szFacilityLst )
    {
        if ( strchr(szFacilityLst,'!') )
        {
            LwStripWhitespace(szFacilityLst, 1, 1);
            
            StripStartAndEndChar(szFacilityLst, ',', NULL);

            pszFirst = strtok_r(szFacilityLst, ")", &pszLast);
            while ( pszFirst )
            {
                pszNext = strstr(pszFirst, "!(");
                if ( pszNext )
                {
                    dwPos = pszNext - pszFirst;

                    if ( dwPos )
                    {
                        strcat(szFacilityString, " and ");
                        strcat(szFacilityString, pszFormatStr);
                        strcat(szFacilityString, "(");
                        strncpy(szFac, pszFirst, dwPos);

                        StripStartAndEndChar(szFac, ',', &dwPos);

                        strncat(szFacilityString, szFac, dwPos);
                        strcat(szFacilityString, ")");
                    }            
                    strcat(szFacilityString, " and not ");
                    strcat(szFacilityString, pszFormatStr);
                    strcat(szFacilityString, "(");

                    pszNext = pszNext + 2;

                    StripStartAndEndChar(pszNext, ',', NULL);

                    strcat(szFacilityString, pszNext);
                    strcat(szFacilityString, ")");
                }
                else
                {
                    dwPos = pszLast - pszFirst;
                    if ( dwPos )
                    {
                        strcat(szFacilityString, " and ");
                        strcat(szFacilityString, pszFormatStr);
                        strcat(szFacilityString, "(");
                        strncpy(szFac, pszFirst, dwPos);

                        StripStartAndEndChar(szFac, ',', &dwPos);

                        strncat(szFacilityString, szFac, dwPos);
                        strcat(szFacilityString, ")");
                    }
                }
                pszFirst = strtok_r(NULL, ")", &pszLast);
            }
        }
        else
        {
            strcpy(szFacilityString, " and ");
            strcat(szFacilityString, pszFormatStr);
            strcat(szFacilityString, "(");

            StripStartAndEndChar(szFacilityLst, ',', NULL);
            
            strcat(szFacilityString, szFacilityLst);
            strcat(szFacilityString, ")");
        }
    }

    if ( *szFilterLst )
    {
        pszFirst = strtok_r(szFilterLst, "]", &pszLast);
        while ( pszFirst )
        {
            if ( (pszNext = strstr(pszFirst, "![")) )
            {
                bNotToken = TRUE;
                pszFirst = pszNext+2;
            }
            else if ( (pszNext = strstr(pszFirst, "[")) )
            {
                bNotToken = FALSE;
                pszFirst = pszNext+1;
            }

            pszStart = strtok_r(pszFirst, ";", &pszEnd);
            while ( pszStart )
            {
                if ( bNotToken )
                {
                    strcat(szFacilityString, " and not filter(");
                    strcat(szFacilityString, pszStart);
                    strcat(szFacilityString, ")");
                }
                else
                {
                    strcat(szFacilityString, " and filter(");
                    strcat(szFacilityString, pszStart);
                    strcat(szFacilityString, ")");
                }
                pszStart = strtok_r(NULL, ";", &pszEnd);
            }
            
            bNotToken = FALSE;
            pszFirst = strtok_r(NULL, "]", &pszLast);
        }
    }

    ceError = LwAllocateString( szFacilityString,
                                &pszFacilityString);
    BAIL_ON_CENTERIS_ERROR(ceError);

cleanup:
    *ppszFacilityStr = pszFacilityString;
    return ceError;

error:
    LW_SAFE_FREE_STRING(pszFacilityString);
    pszFacilityString = NULL;
    goto cleanup;
}

static
CENTERROR 
WriteNGFilter(
    FILE *sngfp,
    PSTR facilities,
    PSTR operator,
    PSTR level,
    PSTR pszMatch,
    PSTR pchFilterName
    )
{
    CENTERROR ceError = 0;
    PSTR pszMatchStr = NULL;
    PSTR pszFacilityStr = NULL;

    ceError = FormatNGFilterString( facilities, 
                                    &pszFacilityStr,
                                    "facility");
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = FormatNGFilterMatchString( pszMatch,
                                         "match",
                                         &pszMatchStr);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if ( !pszMatchStr )
    {
        ceError = LwAllocateString( "",
                                    &pszMatchStr);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if ( !facilities && !level && !operator )
    {
        fprintf( sngfp, 
                 "filter %s {%s; };\n",
                 pchFilterName,
                 pszMatchStr+5);
    }
    else
    {
        if ( !strcmp(facilities, "*")) 
        {
            if ( !strcmp(operator, "and not")) 
            {
                fprintf( sngfp, 
                         "filter %s {not level(%s) %s; };\n",
                         pchFilterName,
                         level,
                         pszMatchStr);

            }
            else 
            {
                fprintf( sngfp, 
                         "filter %s { level(%s) %s; };\n",
                         pchFilterName,
                         level,
                         pszMatchStr);
            }
        }
        else if ( level && operator) 
        {
            fprintf( sngfp, 
                     "filter %s { %s %s level(%s) %s; };\n",
                     pchFilterName,
                     pszFacilityStr+5,
                     operator,
                     level,
                     pszMatchStr);
        }
        else 
        {
            fprintf( sngfp, 
                     "filter %s { %s %s; };\n",
                     pchFilterName,
                     pszFacilityStr+5,
                     pszMatchStr);
        }
    }

error:
    LW_SAFE_FREE_STRING(pszMatchStr);
    LW_SAFE_FREE_STRING(pszFacilityStr);
    return ceError;
} 

static
void
WriteNGLog(
    FILE *sngfp,
    PSTR srcFile,
    PSTR filters,
    PSTR dstPath,
    BOOLEAN bFilter,
    BOOLEAN bActiveLog 
    )
{
    CHAR szLog[8] = {0};

    if ( bActiveLog )
    {
        strcpy(szLog, "log");
    }
    else
    {
        strcpy(szLog, "#log");
    }

    if ( bFilter ) 
    {
        fprintf( sngfp, 
                 "%s { source(%s); filter(%s); destination(%s); };\n\n",
                 szLog,
                 srcFile,
                 filters,
                 dstPath);
    }
    else 
    {
        fprintf( sngfp, 
                 "%s { source(%s); destination(%s); };\n\n",
                 szLog,
                 srcFile,
                 dstPath);
    }
}

static
void
PrepareAndWriteNGFilter(
    FILE* fp,
    PSTR facilityTkn,
    PSTR token,
    PSTR lasts,
    PSTR cFilterName,
    PBOOLEAN pbFilter
    )
{
    char cLevel[20] = {0};
    PSTR pszEnd = NULL;

    //if the token is none 
    if ( !strcmp(token, "none") ) 
    {
        sprintf(cLevel, "debug...emerg"); 
        WriteNGFilter( fp,
                       facilityTkn,
                       "and not",
                       cLevel,
                       lasts,
                       (PSTR )cFilterName); 
    }
    else if ( !strcmp(facilityTkn, "*")  && !strcmp(token, "*")) 
    {
        if ( lasts && strcmp(lasts,""))
        {
            WriteNGFilter( fp,
                           NULL,
                           NULL,
                           NULL,
                           lasts,
                           (PSTR )cFilterName); 
        }
        else
        {
            *pbFilter = FALSE;
        }
    }
    else if ( strspn(token, "=")) 
    {
        token = (PSTR )strtok_r(token, "=", &pszEnd);
        WriteNGFilter( fp,
                       facilityTkn, 
                       "and", 
                       token,
                       lasts,
                       (PSTR )cFilterName); 
    }
    else if ( strspn(token, "!")) 
    {
        if ( strstr(token, "=")) 
        {
            token = (PSTR )strtok_r(token, "!=", &pszEnd);
            WriteNGFilter( fp,
                           facilityTkn,
                           "and not",
                           token,
                           lasts,
                           (PSTR )cFilterName); 
        }
        else 
        {
            token = (PSTR )strtok_r(token, "!", &pszEnd);
            if ( strcmp(token, "emerg") != 0 ) 
            {
                sprintf( cLevel, 
                         "%s...emerg", 
                         (char*)token);
            }
            else 
            {
                sprintf( cLevel,
                         "emerg");
            }

            WriteNGFilter( fp,
                           facilityTkn,
                           "and not",
                           cLevel,
                           lasts,
                           (PSTR )cFilterName); 

        }
    } 
    else if ( strspn(token, "*")) 
    {
        WriteNGFilter( fp,
                       facilityTkn,
                       NULL,
                       cLevel,
                       lasts,
                       (PSTR )cFilterName); 
    }
    else if ( token ) 
    {
        if ( strcmp(token, "emerg") != 0 ) 
        {
            sprintf( cLevel, 
                     "%s...emerg", 
                     (char*)token);
        }
        else 
        {
            sprintf( cLevel,
                     "emerg");
        }

        WriteNGFilter( fp,
                       facilityTkn,
                       "and",
                       cLevel,
                       lasts,
                       (PSTR )cFilterName); 
    }
}

static
void
SplitString(
    FILE *fp, 
    PSTR facilities,
    PSTR cFilterName,
    PBOOLEAN pbActiveLog,
    PBOOLEAN pbFilter    
    )
{
    PSTR facilityTkn = NULL;
    PSTR token = NULL;
    PSTR lasts = NULL;

    facilityTkn = (PSTR)strtok_r(facilities, ".", &lasts);
    token = (PSTR )strtok_r(NULL, ".", &lasts);

    if ( strchr(facilityTkn, '#') )
    {
        PSTR pszFirst = NULL;
        PSTR pszLast = NULL;
        *pbActiveLog = FALSE;
        pszFirst = strtok_r(facilityTkn, "#", &pszLast);
        facilityTkn = pszFirst;
    }
    
    if ( token ) 
    {
        PrepareAndWriteNGFilter( fp,
                                 facilityTkn,
                                 token,
                                 lasts,
                                 cFilterName,
                                 pbFilter);
    }
}

static
CENTERROR
GetFacilityLst(
    PSTR pszInFacility,
    PSTR pszResFacility
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszMark = NULL, pszFirst = NULL, pszLast = NULL;
    DWORD dwPos = 0, dwFacilityLen = 0;

    /* Just remove 'mark' from the facilities as its not 
     * supported in syslog-ng
     */
    pszFirst = strtok_r(pszInFacility, ".", &pszLast);
    if ( pszFirst )
    {
        LwStripWhitespace(pszFirst,1,1);
        pszMark = strstr(pszFirst, "mark");

        if ( pszMark )
        {
            dwPos = pszMark - pszFirst;
            dwFacilityLen = strlen(pszFirst) - 4;

            if ( !dwPos && !dwFacilityLen )
            {
                GPA_LOG_VERBOSE("mark facility is not supported for syslog-ng");
                ceError = CENTERROR_INVALID_OPERATION;
                goto error;
            }

            if ( dwPos )
            {
                strncpy(pszResFacility, pszFirst, dwPos-1);
                pszMark += 4;
                strcat(pszResFacility, pszMark );
            }
            else
            {
                strcpy(pszResFacility, pszMark + 5);
            }
        }
        else
        {
            strcpy(pszResFacility, pszInFacility);
        }
    }

    if ( pszLast )
    {
        strcat(pszResFacility, ".");
        strcat(pszResFacility, pszLast);
    }

error:
    return ceError;
}

static
VOID
SearchAndReplaceFilterInFilter(
    FILE* sngfp,
    PSTR pszInFacility,
    DWORD dwCnt,
    PSTR pszOutFacility,
    PDWORD pdwFilterCnt
    )
{
    PSTR pszNext = NULL;
    BOOLEAN bFilter = FALSE;
    BOOLEAN bActiveLog = FALSE;
    DWORD dwSubCnt = *pdwFilterCnt;
    PSTR pFirst = NULL, pNext = NULL;
    PSTR pszStart = NULL, pszEnd = NULL;
    PSTR pszFirst = NULL, pszLast = NULL;
    CHAR szFacility[FILEREAD_BUFSIZE] = {0};
    CHAR szFilterName[FILEREAD_BUFSIZE] = {0};
    CHAR cFilterName[STATIC_PATH_BUFFER_SIZE] = {0};

    //  ']' may come in match string like *.*.['query']
    //  Functionality added to handle the scenario
    pFirst = strrchr(pszInFacility, '.');
    pNext = strchr(pszInFacility, ']');

    if ( pFirst < pNext )
    {
        strcpy(pszOutFacility, pszInFacility);
        return;
    }

    pszFirst = strtok_r(pszInFacility, "]", &pszLast);
    while ( pszFirst )
    {
        if ( ( pszNext = strchr(pszFirst, '[' ) ) )
        {
            if ( pszNext - pszFirst )
            {
                strncat(pszOutFacility, pszFirst, pszNext - pszFirst); 
            }

            pszNext = pszNext + 1;
            strncpy(szFacility, pszNext, pszLast - pszNext);

            // Processing filter value with semicolon separated string 
            pszStart = strtok_r(szFacility, "+", &pszEnd);
            while ( pszStart )
            {
                sprintf((PSTR)cFilterName, "f_filter_%d%d", dwCnt, dwSubCnt);
                dwSubCnt++;
                
                SplitString( sngfp, 
                             pszStart, 
                             cFilterName,
                             &bActiveLog,
                             &bFilter);
               
                strcat(szFilterName, cFilterName);
                strcat(szFilterName, ";");
                memset(cFilterName, 0, strlen(cFilterName));

                pszStart = strtok_r(NULL, "+", &pszEnd);
            }
            
            strcat(pszOutFacility, "[");
            strcat(pszOutFacility, szFilterName);
            strcat(pszOutFacility,"]");
        }
        else if ( pszLast - pszFirst )
        {
                strncat(pszOutFacility, pszFirst, pszLast - pszFirst);
        }
        
        memset(szFacility, 0 , strlen(szFacility));
        memset(szFilterName, 0, strlen(szFilterName));

        // If match string has '[' and ']' characters then it should not processed. 
        // Condition checks for the match string
        if ( strchr(pszLast, '.') )
        {
            pszFirst = strtok_r(NULL, "]", &pszLast);
        }
        else
        {
            pszFirst = NULL;
        }
    }

    *pdwFilterCnt = dwSubCnt;
}

CENTERROR 
WriteToNGFile (
    FILE *fp, 
    PSTR facilities,
    PSTR pszDestinationPath,
    DWORD dwCnt
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszLast = NULL, pszFacility = NULL, pszFacilityLst = NULL;
    CHAR szFilterName[20] = {0};
    CHAR szDestFilePath[20] = {0};
    CHAR szFacility[FILEREAD_BUFSIZE] = {0};
    CHAR szFacilities[FILEREAD_BUFSIZE] = {0};
    DWORD dwSubCnt = 0;
    BOOLEAN bFilter = TRUE;
    BOOLEAN bActiveLog = TRUE;

    if ( facilities ) 
    {
        ceError = LwAllocateStringPrintf( &pszFacilityLst, 
                                          "%s", 
                                          facilities);
        BAIL_ON_CENTERIS_ERROR(ceError);

        pszFacility = (char *)strtok_r(pszFacilityLst, ";", &pszLast);
    }

    while ( pszFacility ) 
    {
       //Get the facility list without "mark"
        ceError = GetFacilityLst( pszFacility,
                                  szFacilities);

        if ( CENTERROR_SUCCESS == ceError )
        {
            SearchAndReplaceFilterInFilter( fp,
                                            szFacilities,
                                            dwCnt,
                                            szFacility,
                                            &dwSubCnt);

            sprintf((PSTR )szFilterName, "f_filter_%d%d", dwCnt, dwSubCnt);
            sprintf((PSTR )szDestFilePath, "d_filepath_%d%d", dwCnt, dwSubCnt);
            dwSubCnt++;
            
            SplitString( fp, 
                         szFacility, 
                         szFilterName,
                         &bActiveLog,
                         &bFilter);

            WriteNGDest( fp,
                         pszDestinationPath,
                         (PSTR )szDestFilePath,
                         bActiveLog); 

            WriteNGLog( fp, 
                        "src",
                        (PSTR )szFilterName,
                        (PSTR )szDestFilePath,
                        bFilter,
                        bActiveLog); 

            memset(szFilterName, 0, strlen(szFilterName));
            memset(szDestFilePath, 0, strlen(szDestFilePath));
            memset(szFacilities, 0, strlen(szFacilities));
            memset(szFacility, 0, strlen(szFacility));
        }

        pszFacility = (char *)strtok_r(NULL, ";", &pszLast);
    }

error:
    LW_SAFE_FREE_STRING(pszFacilityLst);
    return ceError;
}


CENTERROR
WriteToFile( 
    FILE *pHandle,
    PSTR pszKeyString,
    PSTR pszValueString
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszKey = NULL;
    PSTR pszValue = NULL;
    CHAR szNewKey[FILEREAD_BUFSIZE] = {0};

    ceError = ComputeSyslogKey( pszKeyString, 
                                &pszKey);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = ComputeSyslogValue( pszValueString, 
                                  &pszValue);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (pszKey && pszValue)
    {
        ceError = FormatString( pszKey, 
                                szNewKey);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = GPAFileStreamWrite( pHandle, 
                                      szNewKey, 
                                      strlen(szNewKey));
        BAIL_ON_CENTERIS_ERROR(ceError);
        
        ceError = GPAFileStreamWrite( pHandle, 
                                      "\t\t\t\t", 
                                      4);
        BAIL_ON_CENTERIS_ERROR(ceError);
        
        ceError = GPAFileStreamWrite( pHandle, 
                                      pszValue, 
                                      strlen(pszValue));
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = GPAFileStreamWrite( pHandle, 
                                      "\n", 
                                      1);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:
    LW_SAFE_FREE_STRING(pszKey);
    LW_SAFE_FREE_STRING(pszValue);
    return ceError;
}
