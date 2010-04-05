#include "includes.h"

static PLWT_LOG_INFO pLogInfo;
static PLOG_LEVEL0_DATA pLogLevel0;

static
VOID
InsertLogToLoglist(
    PCSTR pszAPI,
    BOOLEAN bResult
    );

static
VOID
WriteErrorLog( 
    PCSTR pszTestAPIs, 
    PCSTR pszTestDescription, 
    DWORD dwError, 
    PCSTR pszMsg, 
    PCSTR pszFile, 
    PCSTR pszFunction
    );

static
VOID
WriteSuccessLog( 
    PCSTR pszTestAPIs,
    PCSTR pszTestDescription,
    PCSTR pszFile,
    PCSTR pszFunction
    );

static
DWORD
CreateNewNode(
    PLOG_LEVEL0_DATA *ppNewNode, 
    PCSTR pszAPI, 
    BOOLEAN bResult
    );

static
VOID
FreeSyslogList(
    PLOG_LEVEL0_DATA* ppList
    );

static
VOID
WriteLoglistToLogFile(
    );

DWORD
LwtInitLogging(
    PCSTR    pszPath,
    int      nAppend,
    int      nLogLevel
    )
{
    DWORD dwError = 0;

    pLogInfo = calloc (1 , sizeof(LWT_LOG_INFO));

    if ( !pLogInfo )
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        goto error;
    }
   
    pLogInfo->nLogLevel = nLogLevel;
    pLogInfo->pszPath = (PSTR) pszPath;

    if ( IsNullOrEmpty(pszPath) )
    {
        pLogInfo->fp = stdout;
        
    }
    else
    {
        if ( nAppend )
        {
            pLogInfo->fp = fopen(pszPath, "a");
        }
        else
        {
            pLogInfo->fp = fopen(pszPath, "w");
        }

        if ( !pLogInfo->fp )
        {
            dwError = errno;
            goto error;
        }
    }

  cleanup:
     return dwError;

  error:
     goto cleanup;
}


VOID
LwtShutdownLogging(
    VOID
   )
{
    if ( !pLogInfo )
    {
        return;
    }
    
    if ( !pLogInfo->nLogLevel )
    {
        WriteLoglistToLogFile();
        FreeSyslogList(&pLogLevel0);
    }

    if ( pLogInfo->pszPath )
    {
        free(pLogInfo->pszPath);
        pLogInfo->pszPath = NULL;
        
        if ( pLogInfo->fp )
        {
            fclose(pLogInfo->fp);
            pLogInfo->fp = NULL;
        }
    }
    
    free(pLogInfo);
    pLogInfo = NULL;
}

void
LwtLogTest(
    PCSTR pszFunction,
    PCSTR pszFile,
    PCSTR pszTestDescription,
    PCSTR pszTestAPIs,
    DWORD dwError,
    PCSTR pszMsg
    )
{
    switch ( pLogInfo->nLogLevel )
    {
        default:
        case 0:            
            if ( dwError == LW_ERROR_SUCCESS )
            {
                InsertLogToLoglist(pszTestAPIs, 1);
            }
            else
            {
                InsertLogToLoglist(pszTestAPIs, 0);
            }
            break;

        case 1:
            if ( dwError != LW_ERROR_SUCCESS )
            {
                WriteErrorLog( pszTestAPIs, 
                               pszTestDescription, 
                               dwError, 
                               pszMsg, 
                               pszFile, 
                               pszFunction);
            }
            break;

        case 2:
            if ( dwError == LW_ERROR_SUCCESS )
            {
                WriteSuccessLog( pszTestAPIs,
                                 pszTestDescription,
                                 pszFile,
                                 pszFunction);
            }
            else 
            {
                WriteErrorLog( pszTestAPIs,
                               pszTestDescription,
                               dwError,
                               pszMsg,
                               pszFile,
                               pszFunction);
            }
            break;
    }
    
    fflush(pLogInfo->fp);
}

static
VOID
InsertLogToLoglist(
    PCSTR pszAPI,
    BOOLEAN bResult
    )
{
    PLOG_LEVEL0_DATA pNewNode = NULL;
    PLOG_LEVEL0_DATA pTemp = pLogLevel0;
    BOOLEAN bFoundMatch = FALSE;

    CreateNewNode( &pNewNode, 
                   pszAPI, 
                   bResult);

    if ( pNewNode )
    {
        if ( !pTemp )
        {
            pLogLevel0 = pNewNode;
        }
        else
        {
            if ( !strcmp(pTemp->pszApiName, pNewNode->pszApiName) )  
            {
                if ( ( pTemp->bResult != pNewNode->bResult ) && ( 1 == pTemp->bResult ) )
                {
                    pTemp->bResult = pNewNode->bResult;
                }
                
                bFoundMatch = TRUE;
            }
            
            while ( !bFoundMatch && pTemp->pNextLog )
            {
                pTemp = pTemp->pNextLog;

                if ( !strcmp(pTemp->pszApiName, pNewNode->pszApiName) )  
                {
                    if ( ( pTemp->bResult != pNewNode->bResult ) && ( 1 == pTemp->bResult ) )
                    {
                        pTemp->bResult = pNewNode->bResult;
                    }
                    
                    bFoundMatch = TRUE;
                }

            }

            if ( !bFoundMatch ) 
            {
                pTemp->pNextLog = pNewNode; 
            }
            else 
            {
                FreeSyslogList(&pNewNode);
            }
        }
    }
}

static
DWORD
CreateNewNode(
    PLOG_LEVEL0_DATA *ppNewNode, 
    PCSTR pszAPI, 
    BOOLEAN bResult
    )
{
    DWORD dwError = 0;
    PLOG_LEVEL0_DATA pNewNode = NULL;

    pNewNode = calloc(1, sizeof(LOG_LEVEL0_DATA));
    if ( !pNewNode )
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        goto error;
    }
    
    pNewNode->pszApiName = calloc(1, strlen(pszAPI) + 1);
    if ( !pNewNode->pszApiName )
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        goto error;
    }

    strcpy(pNewNode->pszApiName, pszAPI);
    pNewNode->bResult = bResult;
    pNewNode->pNextLog = NULL;

    *ppNewNode = pNewNode;

    return dwError;

error:
    FreeSyslogList(&pNewNode);
    *ppNewNode = NULL;
    return dwError;
}

static
VOID
FreeSyslogList(
    PLOG_LEVEL0_DATA* ppList
    )
{
    PLOG_LEVEL0_DATA pCur = *ppList;
    PLOG_LEVEL0_DATA pTemp = NULL;

    while ( pCur ) 
    {
        pTemp = pCur;
        pCur = pCur->pNextLog;

        if ( pTemp->pszApiName )
        {
            free(pTemp->pszApiName);
            pTemp->pszApiName = NULL;
        }

        free(pTemp);
        pTemp = NULL;
    }

    *ppList = NULL;
}

static
VOID
WriteLoglistToLogFile(
    )
{
    PLOG_LEVEL0_DATA pTemp = pLogLevel0;

    while ( pTemp )
    {
        fprintf(pLogInfo->fp, "API    : %s\n", pTemp->pszApiName);
        if ( pTemp->bResult )
        {
            fprintf(pLogInfo->fp, "Result : Pass\n");                
        }
        else
        {
            fprintf(pLogInfo->fp, "Result : Fail\n");
        }
        fprintf(pLogInfo->fp, "\n");
        fflush(pLogInfo->fp);

        pTemp = pTemp->pNextLog;
    }

}

static
VOID
WriteErrorLog( 
    PCSTR pszTestAPIs, 
    PCSTR pszTestDescription, 
    DWORD dwError, 
    PCSTR pszMsg, 
    PCSTR pszFile, 
    PCSTR pszFunction
    )
{
    char szErrMsg[128] = {0};

    fprintf(pLogInfo->fp, "API         : %s\n", pszTestAPIs);
    fprintf(pLogInfo->fp, "Description : %s\n", pszTestDescription);
    
    if ( dwError == LW_ERROR_TEST_FAILED )
    {
        fprintf(pLogInfo->fp, "Result      : Fail\n");
    }
    else
    {
        fprintf(pLogInfo->fp, "Result      : Broke\n");
    }

    if ( pszMsg )
    {
        sprintf(szErrMsg, "%s", pszMsg);
    }
    else
    {
        sprintf(szErrMsg, "API failed with error code [%lu]", (unsigned long)dwError);
    }

    fprintf(pLogInfo->fp, "Error       : %s\n", pszMsg);
    fprintf(pLogInfo->fp, "File        : %s\n", pszFile);
    fprintf(pLogInfo->fp, "Function    : %s\n", pszFunction);

    fprintf(pLogInfo->fp, "\n");
}

static
VOID
WriteSuccessLog( 
    PCSTR pszTestAPIs,
    PCSTR pszTestDescription,
    PCSTR pszFile,
    PCSTR pszFunction
    )
{
    fprintf(pLogInfo->fp, "API         : %s\n", pszTestAPIs);
    fprintf(pLogInfo->fp, "Description : %s\n", pszTestDescription);
    fprintf(pLogInfo->fp, "Result      : Pass\n");
    fprintf(pLogInfo->fp, "File Name   : %s\n", pszFile);
    fprintf(pLogInfo->fp, "Function    : %s\n", pszFunction);
    fprintf(pLogInfo->fp, "\n");
}
