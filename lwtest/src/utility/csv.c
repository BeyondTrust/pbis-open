#include "includes.h"





static
DWORD
csv_getNthValue(
    PSTR pszRow,
    size_t desiredIndex,
    PSTR *ppszValue
    );

/*
 * Leaves file position at beginning of next row (or eof).
 */
static
DWORD
Csv_CountFields(
    int fd, 
    size_t *pnFields
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    char buf[512];
    ssize_t n;

    size_t i;

    int bDoubleQuoted = 0;
    size_t nFields = 0;

    size_t tBytesProcessed = 0;

    size_t tPosition = 0;
    
    tPosition = lseek(fd, 0, SEEK_CUR);
    /* Count fields, stopping when eof or eol is reached. */
    while ( (n = read(fd, buf, sizeof(buf))) > 0 )
    {
        for ( i = 0; i < n; i++ )
        {
            tBytesProcessed++;
            if ( bDoubleQuoted )
            {
                if ( buf[i] == '"' )
                {
                    bDoubleQuoted = 0;
                }
            }
            else
            {
                if ( buf[i] == '"' )
                {
                    bDoubleQuoted = 1;
                }
                else if ( buf[i] == ',' )
                {
                    nFields++;
                }
                else if ( buf[i] == '\n' )
                {
                    nFields++;
                    lseek(fd, tPosition + tBytesProcessed, SEEK_SET);
                    goto cleanup;
                }
            }
        }
    }

    dwError = LW_ERROR_CSV_BAD_FORMAT;
    goto error;

cleanup:

    *pnFields = nFields;

    return dwError;

error:

    nFields = 0;
    goto cleanup;
}

/* 
 * Leave file pointer at start of next row.
 */
static
DWORD
Csv_ReadRow(
    int fd,
    size_t index,
    PSTR *ppszRow
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    size_t nRowIndex = 0;
    size_t nRowStartOffset = 0;
    size_t nRowEndOffset = 0;

    ssize_t n;

    size_t bytesRead, bytesToRead, bytesToAllocate;

    size_t nFields;
    PSTR pszRow = NULL;

    lseek(fd, 0, SEEK_SET);
    nRowStartOffset = 0;
    nRowIndex = 0;

    while ( nRowIndex < index )
    {
        dwError = Csv_CountFields(fd, &nFields);
        if ( dwError )
            goto error;
        nRowIndex++;
    }

    nRowStartOffset = lseek(fd, 0, SEEK_CUR); 
      
    Csv_CountFields(fd, &nFields);

    nRowEndOffset = lseek(fd, 0, SEEK_CUR);


    /* Copy portion of file from nRowStartOffset to nRowEndOffset
     * into a string.
     */
    bytesRead = 0;
    bytesToRead = (nRowEndOffset - nRowStartOffset);
    if ( bytesToRead == 0 ) /* Nothing more; return success and a NULL*/
        goto cleanup;

    bytesToAllocate = bytesToRead + 1;
    pszRow = malloc(sizeof(char) * bytesToAllocate);
    if ( ! pszRow )
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        goto error;
    }

    pszRow[0] = '\0';
    if ( bytesToRead > 0 )
    {
        lseek(fd, nRowStartOffset, SEEK_SET);
        while ( 
                (n = read(fd, pszRow + bytesRead, bytesToRead)) 
                    < bytesToRead 
              )
        {
            if ( n == 0 )
                break;
            bytesToRead -= n;
            bytesRead += n;

            n = 0;
        }
        if ( n > 0 )
        {
            bytesToRead -= n;
            bytesRead += n;
        }
        pszRow[bytesRead - 1] = '\0';
    }

cleanup:

    *ppszRow = pszRow;

    return dwError;

error:

    if ( pszRow )
    {
        free(pszRow);
        pszRow = NULL;
    }

    goto cleanup;
}


/*
 * CsvOpenFile
 *
 */
DWORD
CsvOpenFile(
    PCSTR pszFilename,
    PVOID *ppCsv
    )
{
    
    DWORD dwError = LW_ERROR_SUCCESS;
    PLWTCSV pCSV = NULL;

    int fd = -1;
    size_t nFields = 0;
    PSTR* ppszFieldNames = NULL;
    size_t nRowCount = 0;

    PSTR pszRow = NULL;

    size_t i = 0;

    fd = open(pszFilename, O_RDONLY, 0);
    if ( fd == -1 )
    {
      dwError = LwMapErrnoToLwError(errno);
      goto error;
    }

    dwError = Csv_CountFields(fd, &nFields);
    if ( dwError )
        goto error;

    ppszFieldNames = calloc(nFields, sizeof(PSTR));
    if ( ! ppszFieldNames )
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        goto error;
    }

    dwError = Csv_ReadRow(fd, 0, &pszRow);
    if ( dwError )
        goto error;

    for ( i = 0; i < nFields; i++ )
    {
        PSTR pszValue = NULL;
        dwError = csv_getNthValue(pszRow, i, &pszValue);
        if ( dwError )
            goto error;

        ppszFieldNames[i] = pszValue;
    }
    free(pszRow);
    pszRow = NULL;


    /* Now determine number of rows. */
    while ( 
            (dwError = Csv_ReadRow(fd, nRowCount + 1, &pszRow)) 
                == LW_ERROR_SUCCESS &&
            pszRow != NULL 
          )
    {
        nRowCount++;
        free(pszRow);
        pszRow = NULL;
    }
    dwError = LW_ERROR_SUCCESS;

    pCSV = malloc(sizeof(LWTCSV));
    if ( ! pCSV ) {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        goto error;
    }
    pCSV->nFields = nFields;
    pCSV->ppszFieldNames = ppszFieldNames;

    pCSV->nRows = nRowCount;
    pCSV->fd = fd;

    *ppCsv = pCSV;
    return dwError;
  
error:
    if ( fd != -1)
    {
        close(fd);
        fd = -1;
    }
    if ( pszRow )
        free(pszRow);

    if ( ppszFieldNames )
    {
        for ( i = 0; i < nFields; i++ )
            free(ppszFieldNames[i]);
        free(ppszFieldNames);
        ppszFieldNames = 0;
    }
    return dwError;
}

DWORD
CsvGetRowCount(
    PVOID pvContext
    )
{
    PLWTCSV pCsv = (PLWTCSV)pvContext;
    return (DWORD) pCsv->nRows;
}

DWORD
CsvGetRow(
    PLWTCSV pCsv,
    size_t index,
    PSTR *ppszRow
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    PSTR pszRow = NULL;

    *ppszRow = NULL;

    dwError = Csv_ReadRow(pCsv->fd, index + 1, &pszRow);

    if ( ! dwError )
        *ppszRow = pszRow;

    return dwError;
}


DWORD
CsvGetValue(
    PLWTCSV pCsv,
    PSTR pszRow,
    PCSTR pszFieldName,
    PSTR *ppszValue
    )
{
    DWORD dwError = LW_ERROR_CSV_NO_SUCH_FIELD;

    PSTR pszValue = NULL;

    size_t i;

    for ( i = 0; i < pCsv->nFields; i++)
    {
        if ( !strcmp(pCsv->ppszFieldNames[i], pszFieldName))
        {
            dwError = csv_getNthValue(pszRow, i, &pszValue);
            if ( dwError )
                goto error;
            else
                goto cleanup;
        }
    }

cleanup:
    *ppszValue = pszValue;
    return dwError;

error:
    free(pszValue);
    pszValue = NULL;
    goto cleanup;
}

static
DWORD
csv_getNthValue(
    PSTR pszRow,
    size_t desiredIndex,
    PSTR *ppszValue
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSTR pszValue = NULL;

    int bDoubleQuoted = 0; /* true if we are in a double quoted string. */
    size_t currentIndex = 0;

    size_t nFieldStart = 0;
    size_t nFieldEnd = 0;

    size_t i;

    for ( i = 0; pszRow[i]; i++)
    {
        if ( bDoubleQuoted )
        {
            if ( pszRow[i] == '"' )
            {
                bDoubleQuoted = 0;
            }
        }
        else
        {
            if ( pszRow[i] == ',' || pszRow[i] == '\n' )
            {
                if ( currentIndex == desiredIndex )
                {
                    nFieldEnd = i;
                    goto found_field;
                }
                else
                {
                    currentIndex++;
  
                    if ( currentIndex == desiredIndex )
                        nFieldStart = i + 1;
                }
            }
            else if ( pszRow[i] == '"' )
            {
                bDoubleQuoted = 1;
            }
        }
    }

    if ( bDoubleQuoted )
    {
        dwError = LW_ERROR_CSV_BAD_FORMAT;
        goto error;
    }
    if ( currentIndex != desiredIndex )
    {
        dwError = LW_ERROR_CSV_NO_SUCH_FIELD;
        goto error;
    }
    nFieldEnd = i;

found_field:

    /*  
     *                ,xxxxxxxxxxxxx,
     *  nFieldStart ---^            ^--- nFieldEnd
     *              xxx,,xxxxx
     *  nFieldStart ----^-----nFieldEnd    (empty -- return NULL )
     */

    /* If we have an empty field not double quoted, return NULL. */
    if ( pszRow[nFieldStart] != '"' && nFieldStart == nFieldEnd )
    {
        goto cleanup;
    }

    if ( pszRow[nFieldStart] == '"' )
    {
        if ( pszRow[nFieldEnd - 1] != '"' )
        {
            dwError = LW_ERROR_CSV_BAD_FORMAT;
            goto error;
        }

        nFieldStart += 1;
        nFieldEnd -= 1;
    }

    pszValue = malloc(sizeof(char) * ((nFieldEnd - nFieldStart) + 1));
    if ( ! pszValue )
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        goto error;
    }

    memcpy(pszValue, pszRow + nFieldStart, nFieldEnd - nFieldStart);
    pszValue[(nFieldEnd - nFieldStart)] = '\0';

cleanup:

    *ppszValue = pszValue;
    return dwError;

error:

    free(pszValue);
    pszValue = NULL;

    goto cleanup;
}

DWORD
CsvFree(
   PSTR pszRowOrValue
    )
{
    free(pszRowOrValue);
    return LW_ERROR_SUCCESS;
}

DWORD
CsvCloseFile(
    PVOID pvCsv
    )
{
    size_t i;
    PLWTCSV pCsv = (PLWTCSV)pvCsv;

    if ( pCsv->fd != -1 )
    {
        close(pCsv->fd);
        pCsv->fd = -1;
    }

    for ( i = 0; i < pCsv->nFields; i++)
        free(pCsv->ppszFieldNames[i]);
    free(pCsv->ppszFieldNames);
    free(pCsv);

    return LW_ERROR_SUCCESS;
}

DWORD 
Csv_LoadInterface(
        PLWTDATAIFACE *ppLwtDataIface
                )
{


    PLWTDATAIFACE pLwtIface;
    DWORD dwError = LW_ERROR_SUCCESS;


    pLwtIface = malloc(sizeof(LWTDATAIFACE));
    if (pLwtIface == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL(dwError);
    }

    pLwtIface->Init = CsvOpenFile;
    pLwtIface->GetNext = CsvGetNext;
    pLwtIface->GetMaxEntries = CsvGetRowCount;
    pLwtIface->ContextFree = CsvCloseFile;
    //pLwtIface->Lwt_GetValue = CsvGetValue;
    pLwtIface->Reset = CsvReset;

cleanup:
    *ppLwtDataIface = pLwtIface;
    return dwError;

error:
    if (pLwtIface)
    {
        free(pLwtIface);
        pLwtIface = NULL;
    }

    goto cleanup;

}
   

DWORD
CsvGetNext(
        PVOID pvCsv,
        size_t tRow,
        PLWTDATA *ppData
        )
{
    PSTR *ppszFieldValue = NULL;
    PSTR *ppszFieldName  = NULL;
    PSTR pszFieldValue   = NULL;
    PSTR pszFieldName    = NULL;
    PSTR pszRow          = NULL;
    PLWTDATA pData       = NULL;


    PLWTCSV pCsv = (PLWTCSV) pvCsv;
    size_t tFieldCount = 0;
    DWORD dwError = 0;
  
       

    pData = malloc(sizeof(LWTDATA));
    if (pData == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL(dwError);;
    }
    
    ppszFieldName = calloc(pCsv->nFields, sizeof(PSTR));
    if (ppszFieldName == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL(dwError);
    }
   
    ppszFieldValue = calloc(pCsv->nFields, sizeof(PSTR));
    if (ppszFieldValue == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL(dwError);
    }
    


    dwError = CsvGetRow(pCsv, tRow, &pszRow);
    BAIL(dwError);
    
    for (tFieldCount =0; tFieldCount < pCsv->nFields; tFieldCount++)
    {
        
        DWORD dwLen = 0;

        dwError = CsvGetValue(pCsv, pszRow, pCsv->ppszFieldNames[tFieldCount], &pszFieldValue);
        BAIL(dwError);
        ppszFieldValue[tFieldCount] = pszFieldValue;
        
        dwLen = strlen(pCsv->ppszFieldNames[tFieldCount]);
        pszFieldName = malloc(dwLen+1);
        if (pszFieldName == NULL)
        {
            dwError = LW_ERROR_OUT_OF_MEMORY;
            BAIL(dwError);
        }
        memcpy(pszFieldName, pCsv->ppszFieldNames[tFieldCount], dwLen);
        pszFieldName[dwLen] = '\0';
        ppszFieldName[tFieldCount] = pszFieldName;
        
    }   

cleanup:
    
    if (pszRow)
    {
        free(pszRow);
        pszRow = NULL;
    }

    pData->ppszFieldName = ppszFieldName;
    pData->ppszFieldValue = ppszFieldValue;
    pData->nFields = pCsv->nFields;
    *ppData = pData;
    
    return dwError;
error:

    if(dwError != LW_ERROR_SUCCESS)
    {
        free(pszFieldValue);
        pszFieldValue = NULL;
    }
    
    for (tFieldCount =0; tFieldCount < pCsv->nFields; tFieldCount++)
    {
        if(ppszFieldName)
        {
            if (ppszFieldName[tFieldCount])
            {
                free(ppszFieldName[tFieldCount]);
                ppszFieldName[tFieldCount] = NULL;
            }
            free(ppszFieldName);
            ppszFieldName = NULL;
        }

        if (ppszFieldValue)
        {
            if (ppszFieldValue[tFieldCount])
            {
                free(ppszFieldValue[tFieldCount]);
                ppszFieldValue[tFieldCount] = NULL;
            }
            free(ppszFieldValue);
            ppszFieldValue = NULL;
        }
    }
    goto cleanup; 
}

DWORD
CsvReset(PVOID pvCsv)
{
    PLWTCSV pCsv = (PLWTCSV)pvCsv;
    lseek(pCsv->fd, 0, SEEK_SET);
    return LW_ERROR_SUCCESS;
}

