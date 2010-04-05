#include <includes.h>

static
VOID 
RemoveFilterInFilter( 
    PSTR pszTemp,
    PSTR pszKey
    )
{
    PSTR pszFirst = NULL, pszLast = NULL, pszNext = NULL;
    DWORD dwPos = 0;

    // Skip filter in filter contents
    pszFirst = strtok_r(pszTemp, "]", &pszLast);
    while ( pszFirst )
    {
        if ( ( pszNext = strstr(pszFirst, "![") ) || 
             ( pszNext = strstr(pszFirst, "[") ) )
        {
            dwPos = pszNext - pszFirst;
            if ( dwPos )
            {
                if (pszFirst[dwPos-1] == ';')
                {
                    strncat(pszKey, pszFirst, dwPos);
                }
                else
                {
                    strncat(pszKey, pszFirst, dwPos - 1);
                }
            }
        }
        else
        {
            strcat(pszKey, pszFirst);
        }

        pszFirst = strtok_r(NULL, "]", &pszLast);
    }
}

static
VOID 
GetSyslogKey( 
    PSTR pszKey, 
    PSTR pszOutKey
    )
{
    PSTR pszStart = NULL, pszEnd = NULL;
    PSTR pszLast = NULL, pszFirst = NULL, pszNext = NULL;
    CHAR szActualKey[FILEREAD_BUFSIZE] = {0};
    CHAR szStart[FILEREAD_BUFSIZE] = {0};

    if(GPAStrStartsWith(pszKey,"ifdef"))
    {
        strcpy(pszOutKey, pszKey);
    }
    else
    {
        pszStart = strtok_r(pszKey, ";", &pszEnd);
        while ( pszStart )
        {
            if (*pszOutKey)
            {
                strcat(pszOutKey, ";");
            }

            if ( StringStartsWithChar(pszStart, '.'))
            {
                strcat(szStart, "*");
            }
            strcat(szStart, pszStart);
            
            // Skip match match , If *.*.match is present ignore the entry
            pszFirst = strtok_r(szStart, ".", &pszLast);
            pszNext = strtok_r(NULL, ".", &pszLast);

            if ( pszFirst )
            {
                if (*pszFirst == '\0')
                {
                    strcat (szActualKey, "*");
                }
                else
                {
                    strcat (szActualKey, pszFirst);            
                }
            }

            if ( pszNext )
            {
                strcat (szActualKey, ".");
                if(*pszNext)
                {
                    strcat (szActualKey, pszNext);
                }
                else
                {
                    strcat (szActualKey, "*");
                }
            }

            if ( pszLast && *pszLast )
            {
                if ( !strcmp(szActualKey, "*.*"))
                {
                    // Scenario: *.*.match() - Skipping complete key otherwise interpret will result the key as *.*
                    memset(szActualKey, 0, strlen(szActualKey));
                }
            }        

            strcat(pszOutKey, szActualKey);

            memset(szStart, 0, strlen(szStart));
            memset(szActualKey, 0, strlen(szActualKey));

            pszStart = strtok_r(NULL, ";", &pszEnd);
        }
    }
}

CENTERROR 
ComputeSyslogKey(
    PSTR pszKeyString,
    PSTR *ppszKey
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszKey = NULL, pszTemp = NULL;
    CHAR szKey[FILEREAD_BUFSIZE] = {0};
    CHAR szResultKey[FILEREAD_BUFSIZE] = {0};

    ceError = LwAllocateString( pszKeyString,
                                &pszTemp);
    BAIL_ON_CENTERIS_ERROR(ceError);

    RemoveFilterInFilter( pszTemp,
                          szKey);

    GetSyslogKey( szKey, 
                  szResultKey);

    if ( *szResultKey )
    {
        ceError = LwAllocateString( szResultKey,
                                    &pszKey);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    *ppszKey = pszKey;

cleanup:
    LW_SAFE_FREE_STRING(pszTemp);
    return ceError;

error:
    LW_SAFE_FREE_STRING(pszKey);
    *ppszKey = NULL;
    goto cleanup;
}

CENTERROR 
ComputeSyslogValue(
    PSTR pszValueString,
    PSTR *ppszValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszValue = NULL, pszFirst = NULL;
    PSTR pszLast = NULL, pszTemp = NULL;
    
    if( GPAStrStartsWith(pszValueString,"ifdef") )
    {
        ceError = LwAllocateString( pszValueString,
                                    &pszValue);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    else
    {
        ceError = LwAllocateString( pszValueString,
                                    &pszTemp);
        BAIL_ON_CENTERIS_ERROR(ceError);

        LwStripWhitespace(pszTemp,1,1); 
        
        pszFirst = strtok_r(pszTemp, " ", &pszLast);

        LwStripWhitespace(pszFirst,1,1); 

        if (pszFirst && *pszFirst)
        {
            ceError = LwAllocateString( pszFirst,
                                        &pszValue);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        else if (pszFirst == NULL)
        {
            ceError = LwAllocateString( "  ",
                                        &pszValue);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

    }

    *ppszValue = pszValue;

cleanup:
    LW_SAFE_FREE_STRING(pszTemp);
    return ceError;

error:
    LW_SAFE_FREE_STRING(pszValue);
    *ppszValue = NULL;
    goto cleanup;
}

CENTERROR
AppendEntryToList(
    PSTR pszKey,
    PSTR pszValue,
    PSYSLOGNODE* ppListFromAD
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bDuplicateEntry = FALSE;
    PSYSLOGNODE pNewNode = NULL;

    IsDuplicateEntry( ppListFromAD, 
                      pszKey,
                      pszValue,
                      &bDuplicateEntry);

    if ( !bDuplicateEntry )
    {
        ceError = CreateNewNode( &pNewNode, 
                                 pszKey, 
                                 pszValue);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = AppendNodeToList( ppListFromAD, 
                                    pNewNode);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:
    return ceError;
}

CENTERROR
PrepareListFromFile(
    PCSTR pszOrigFile,
    PSYSLOGNODE *ppMerged
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSYSLOGNODE pNewNode = NULL;
    CHAR szLine[FILEREAD_BUFSIZE] = {0};
    CHAR szTmpLine[FILEREAD_BUFSIZE] = {0};
    BOOLEAN bAppendLine = FALSE;
    BOOLEAN bAppendIfDef = FALSE;
    PSTR pszKey = NULL, pszValue = NULL, pszToken = NULL;
    FILE* fpFile = NULL;

    ceError = GPAOpenFile( pszOrigFile,
                          "r",
                          &fpFile);
    BAIL_ON_CENTERIS_ERROR(ceError);

    while ( fgets(szLine, FILEREAD_BUFSIZE, fpFile) ) 
    {
        StripTabspace(szLine );

        LwStripWhitespace(szLine,1,1);        

        if ( strlen(szLine) && !StringStartsWithChar(szLine,'#') ) 
        {
            if(GPAStrStartsWith(szLine,"ifdef"))
            {
                strcpy(szTmpLine,szLine);
                strcat(szTmpLine, "\n");
                bAppendIfDef = TRUE;
            }
            else if ( bAppendIfDef && !strchr(szLine,')' ))
            {
                strcat(szTmpLine, szLine);
                strcat(szTmpLine, "\n");
                strcat(szTmpLine, "\0");
            }
            else if (strchr(szLine,'\\') )
            {
                bAppendLine = TRUE;
                pszToken = strchr(szLine, '\\');
                strncat(szTmpLine, szLine, pszToken - szLine);
                strcat(szTmpLine, "\0");
            }
            else
            {
                if ( bAppendLine )
                {
                    strcat(szTmpLine, szLine);
                    bAppendLine = FALSE;
                    pszKey = strtok_r(szTmpLine," ",&pszValue );
                }
                else if ( bAppendIfDef && strchr(szLine, ')') )
                {
                    CHAR szTmpVal[] = "  ";
                    strcat(szTmpLine, szLine);
                    bAppendIfDef = FALSE;

                    pszKey = szTmpLine;
                    pszValue = szTmpVal;
                }
                else   
                {
                    strcpy(szTmpLine, szLine );
                    pszKey = strtok_r(szTmpLine," ",&pszValue );
                }

                ceError = CreateNewNode( &pNewNode, 
                                         pszKey, 
                                         pszValue);
                BAIL_ON_CENTERIS_ERROR(ceError);

                ceError = CompareAndAppendNode( ppMerged, 
                                                pNewNode);
                BAIL_ON_CENTERIS_ERROR(ceError);

                memset(szLine, 0, strlen(szLine));
                memset(szTmpLine, 0, strlen(szTmpLine));
            }
        }
    }

error:
    if ( fpFile )
    {
        GPACloseFile(fpFile);
        fpFile = NULL;
    }
    return ceError;
}


CENTERROR
FormatString(
    PSTR pszStr,
    PSTR pszNewStr
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszStart = NULL;
    PSTR pszEnd = NULL;
    PSTR pszVal = NULL;

    ceError = LwAllocateStringPrintf( &pszVal,
                                      "%s\n",
                                      pszStr);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if ( strlen(pszStr) > 24 )
    {
        pszStart = strtok_r(pszVal, ";", &pszEnd);

        while ( pszStart )
        {
            strcat(pszNewStr,pszStart);

            if ( !strchr(pszStart,'\n') )
            {
                strcat(pszNewStr,";\\");
                strcat(pszNewStr,"\n\t");
            }
            else
            {
                LwStripWhitespace(pszNewStr,1,1);
            }

            pszStart = strtok_r(NULL, ";", &pszEnd);
        }
    }
    else
    {
        strcpy(pszNewStr,pszStr);
    }

error:

    LW_SAFE_FREE_STRING(pszVal);

    return ceError;
}
