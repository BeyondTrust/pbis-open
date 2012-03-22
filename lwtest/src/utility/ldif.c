/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 *  * -*- mode: c, c-basic-offset: 4 -*- */

/*
 *   Copyright Likewise Software    2009
 *   All rights reserved.
 *  
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *   
 *    This program is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *    for more details.  You should have received a copy of the GNU General
 *    Public License along with this program.  If not, see
 *    <http://www.gnu.org/licenses/>.
 *    
 *    LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 *    TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 *    WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 *    TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 *    GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 *    HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 *    TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 *    license@likewisesoftware.com
 */
 

 /*                                                              *
 *
 *   Utiltiy API for the LDIF file to read the data                                                      
 *                                                               *  
 ******************************************************************/



#include "includes.h"




/*
 *
 * Opens the Ldif file named pszFile Name, 
 * Initailizes other paraneter of the structure.
 *
 */
static
DWORD 
LdifInit(
    PCSTR szFileName,
    PVOID *ppLdif
    );


static 
DWORD 
CopyBuffer(
    PSTR pszBuf, 
    PSTR *ppStrValue, 
    size_t tLength
    );


/*
 *
 * Reads the field buffer parses the field for a user
 * stores it in the key value structure
 */

static
DWORD
LdifGetNext(
    PVOID pvLdif,
    size_t nIndex,
    PLWTDATA *ppData
    );


/*
 *
 *
 * COunt the key value pairs in the next line
 *
 */
static
DWORD
LdifCountNoOfFieldsnEntries(
            PLWTLDIF pLdif,
            size_t *dwMaxFields,
            size_t *dwMaxEntries,
            size_t nOption
            );


static
DWORD
LdifGetMaxEntries(
            PVOID pvLdif
           );
/*
 *
 *
 * Retruns a value for the key sroted in pszFieldName
 *

DWORD
LdifGetValue(
    PLWTDATA pLdifData,
    PCSTR pszFieldName,
    PSTR *ppszFieldValue
    );*/



/*
 *
 * PLdif frees the data refered by ppData
 */
static
DWORD
LdifFree(
    PVOID pData
    );


DWORD 
LdifReset(
        PVOID pvLdif
        );


static
DWORD
LdifGetMaxEntries(
            PVOID pvLdif
           )
{
    PLWTLDIF pLdif = (PLWTLDIF) pvLdif;
    return pLdif->nMaxEntries;
}


DWORD 
Ldif_LoadInterface(
         PLWTDATAIFACE *ppDataIface
            )
{
    PLWTLDIF pLdif = NULL;
    PLWTDATAIFACE pLwtIface = NULL;
    DWORD dwError = LW_ERROR_SUCCESS;

    pLwtIface = malloc(sizeof(LWTDATAIFACE));
    if(pLwtIface == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL(dwError);
    }
    
    pLdif = malloc(sizeof(LWTLDIF));
    if (pLdif == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL(dwError);
    }
    
    pLwtIface->pvDataContext = pLdif;
    pLwtIface->Init = LdifInit;
    pLwtIface->GetNext = LdifGetNext;
    pLwtIface->GetMaxEntries = LdifGetMaxEntries;
    pLwtIface->ContextFree = LdifFree;
    pLwtIface->Reset = LdifReset;

cleanup:
    *ppDataIface = pLwtIface;
    return dwError;

error:
    if (pLdif)
    {
        free(pLdif);
        pLdif = NULL;
    }
      
    if (pLwtIface)
    {
        free(pLwtIface);
        pLwtIface = NULL;
    }
    goto cleanup;

}



/*
 * Initailises Ldif file properties
 *
 */
static
DWORD
LdifInit(
    PCSTR pszFileName,
    PVOID *ppLdif
    )
{

    DWORD dwError = LW_ERROR_SUCCESS;
    size_t nMaxEntries = 0;
    size_t nMaxFieldCount = 0;
    
    int fd = 0;
    PLWTLDIF pLdif = NULL;

    fd = open(pszFileName,O_RDONLY, 0);
    if(fd == -1)
    {
        return -1;
    }


    pLdif = malloc(sizeof(LWTLDIF));
    if (pLdif == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        return dwError;
    }

    pLdif->fd = fd;
    pLdif->tPosition = 0;
    dwError = LdifCountNoOfFieldsnEntries(pLdif, &nMaxFieldCount, &nMaxEntries, 2);
    BAIL(dwError);


cleanup:
    pLdif->nMaxEntries = nMaxEntries;
    pLdif->nFields = nMaxFieldCount;
    *ppLdif = pLdif;
    return dwError;

error:
    if (pLdif)
    {
        free(pLdif);
        pLdif = NULL;
    }
    goto cleanup;
}

/*
 *
 *
 * Utility function to count the no of fields in the 
 * file for allocation purpose
 */            
   
static
DWORD
LdifCountNoOfFieldsnEntries(
            PLWTLDIF pLdif,
            size_t *nMaxFieldCount,
            size_t *nMaxEntries,
            size_t nOption
            )
{

    char pszBuf[512];
    BOOL bFieldEnded = 0, bLineSeparator = 0, bLineFeed = 0;
    BOOL bFieldNameEnded = 0;
    DWORD dwError = LW_ERROR_SUCCESS;
    int nBufCount = 0;
    size_t tNumberOfField = 0;
    size_t tNumberOfEntries = 0, tSeekPosition = pLdif->tPosition;
    size_t tLineEnded = 0, i = 0;



    /* Variable lenght fields..Make sure you count them right for initial entire reading, then field by field counts*/
    if(nOption == 1)
    {
        lseek(pLdif->fd, pLdif->tPosition, SEEK_SET);
    }
    else
    {
        lseek(pLdif->fd, 0, SEEK_SET);    
    }

    /*TODO: revisit this code and review it*/
    while ((nBufCount = read(pLdif->fd, pszBuf, sizeof(pszBuf))) > 0)
    {
        
        for ( i = 0; i < nBufCount; i++)
        {
           //Three cases 
           //Line started/r/n
           //Line ended  /r/n
           //Field ended /r/n/n
            switch (pszBuf[i])
            {
                case '\r' :
                        bLineFeed = 1;
                        break;
                case '\n' :
                        if (bLineFeed == 1)
                        {
                            if(bLineSeparator == 1)
                            {
                                bFieldEnded = 1;
                                tNumberOfField++;
                                tLineEnded = i -1;
                                break;
                            }
                            else
                            {
                                bLineSeparator = 1;
                            }
                        }
                        else
                        {
                            dwError = LW_ERROR_CSV_BAD_FORMAT;
                            BAIL(dwError)
                        }
                        break;
                case ':':
                         if (bFieldNameEnded == 0)
                         {
                            bFieldNameEnded = 1;
                         }
                         break; 
                case ' ' :
                        if (bLineSeparator == 1)
                        {
                            /* Folded Line */
                            bLineSeparator = 0;
                        }
                        break;

                default :
                        if (bLineSeparator == 1)
                        {
                            if( tLineEnded != i -2)
                            {
                                tNumberOfField++;
                                tLineEnded = i -2;
                            }
                        }
                        bLineSeparator = 0;            
            }
            if (bFieldEnded == 1)
            {
                if (!*nMaxFieldCount)
                {
                    *nMaxFieldCount = tNumberOfField;

                    /* used only for reading the number of fields in an entry*/
                    if (nOption == 1)
                    {
                        goto cleanup;
                    }
                }
                bFieldEnded = 0;
                tNumberOfEntries++;
            }

        }
        /* If the read buffer ends right in end of the field*/
        if (nBufCount == 512)
        { 
            tSeekPosition += tLineEnded;
            bLineSeparator = 0;
            bLineFeed = 0;
            lseek(pLdif->fd, tSeekPosition, SEEK_SET);
            tLineEnded = 0;
        }
    }
cleanup:
   if (nOption == 2)
    {
       *nMaxEntries = tNumberOfEntries;
    }
    return dwError; 
error:
    goto cleanup;
}







/*
 *
 * Parser for LDIF, it parses the file contents as key value pairs
 * stores it in the PLWTDATA structure
 *
 */
static 
DWORD
LdifGetNext(
    PVOID pvLdif,
    size_t nIndex,
    PLWTDATA *ppData
    )

{

    PLWTLDIF pLdif = (PLWTLDIF) pvLdif;
    char pszBuf[512];
    size_t tLineStarted = 0, tLineEnded = 0;
    BOOL bLineFeed = 0, bFieldEnded = 0, bLineSeparator= 0;
    BOOL bBase64Decode = 0;
    size_t tCurIndex = 0;
    size_t tFieldValueStarted = 0;
    size_t tTotalNoOfFields = 0, tTotalEntries;
    size_t tFieldNameEnded = 0, tFieldLength = 0;
    DWORD dwError = LW_ERROR_SUCCESS;
    int nBufCount = 0, i = 0;
    PLWTDATA pData = NULL;
    size_t tLength;
    PSTR pszFieldName = NULL;
    PSTR pszFieldValue = NULL;

    //Reset

    if(nIndex == 0)
    {
        pLdif->tPosition = 0;
    }
    lseek(pLdif->fd, pLdif->tPosition, SEEK_SET);
    


    /* Count the no of fields in this block*/
    LdifCountNoOfFieldsnEntries(pLdif, &tTotalNoOfFields, &tTotalEntries , 1);
    pLdif->nFields = tTotalNoOfFields;

    tTotalNoOfFields = 0; /*Just for testing remove this code here n inside the loop*/


    //Copy the data in to pData;
    pData = malloc(sizeof(PLWTDATA));
    if (pData == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL(dwError);
    }

    pData->ppszFieldName = calloc(pLdif->nFields, sizeof(char **));
    if(pData->ppszFieldName == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL(dwError);
    }
    

    pData->ppszFieldValue = calloc(pLdif->nFields, sizeof(char **));
    if(pData->ppszFieldValue == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL(dwError);
    }
    

    /*Go back to the start of the position before counting */
    lseek(pLdif->fd, pLdif->tPosition, SEEK_SET);
    while ((nBufCount = read(pLdif->fd, pszBuf, sizeof(pszBuf))) > 0)
    {
        tLineStarted = 0;
        tFieldLength = 0;
        tFieldValueStarted = 0;
        tFieldNameEnded = 0;
        tLineEnded = 0;
        tLength = 0;
        
        for (i = 0; i < nBufCount; i++)
        {
           //Three :wcases 
           //Line started/r/n
           //Line ended  /r/n
           //Field ended /r/n/n
            switch (pszBuf[i])
            {
                case '\r' :
                    bLineFeed = 1;
                    break;

                case '\n' :
                        if (bLineFeed == 1)
                        {
                            if (bLineSeparator == 1)
                            {
                                bFieldEnded = 1;
                                
                                tLineEnded = i -1;

                                //Copy the last field and value here
                                tLength = tFieldNameEnded - tLineStarted;
                                dwError = CopyBuffer((pszBuf+tLineStarted), &pszFieldName, tLength);
                                pszFieldName = malloc(tLength+1);
                                BAIL(dwError);
                                pData->ppszFieldName[tCurIndex] = pszFieldName; 
                                 
                                tLength = tLineEnded - tFieldValueStarted;
                                dwError = CopyBuffer((pszBuf+tFieldValueStarted), &pszFieldValue, tLength);
                
                                BAIL(dwError);


                                /*if (bBase64Decode)
                                {
                                    DWORD dwKeyLength = 0;
                                    PSTR pszDecodedString = NULL;
                                    dwError = Base64Decode(pszFieldValue, &pszDecodedString, &dwKeyLength);
                                    BAIL(dwError);

                                    free(pszFieldValue);
                                    pszFieldValue = pszDecodedString;
                                    bBase64Decode = 0;
                                
                                }*/
                                pData->ppszFieldValue[tCurIndex] = pszFieldValue;
                                tCurIndex++;
                                tTotalNoOfFields++;
                                bLineSeparator = 0;
                            
                            }
                            else
                            {
                                bLineSeparator = 1;
                            }
                        }
                        else
                        {
                            dwError = LW_ERROR_CSV_BAD_FORMAT;
                            BAIL(dwError)
                        }
                        break;
                case ':' :
                        if (i == tFieldNameEnded+1) 
                        {
                            bBase64Decode = 1;
                        }
                        else
                        {
                            tFieldNameEnded = i;
                        }
                        tFieldValueStarted = i+2;
                        break;

                case ' ' :
                        if (bLineSeparator == 1)
                        {
                            /* Folded Line */
                            bLineSeparator = 0;
                        }
                        break;

                default :
                        if (bLineSeparator == 1)
                        {
                            if (tLineEnded != i -2)
                            {
                                /* Copy the field from start of the line to end */
                                tLineEnded = i-2;
                                tLength = tFieldNameEnded - tLineStarted;
                                dwError = CopyBuffer((pszBuf+tLineStarted), &pszFieldName, tLength);
                                BAIL(dwError);
                                pData->ppszFieldName[tCurIndex]=pszFieldName;
                            
                                tLength = tLineEnded - tFieldValueStarted;
                                dwError = CopyBuffer((pszBuf+tFieldValueStarted), &pszFieldValue, tLength);
                                BAIL(dwError);
    
                                /*if (bBase64Decode)
                                {
                                    DWORD dwKeyLength = 0;
                                    PSTR pszDecodedString = NULL;
                                    dwError = Base64Decode(pszFieldValue, &pszDecodedString, &dwKeyLength);
                                    BAIL(dwError);
                                    free(pszFieldValue);
                                    pszFieldValue = pszDecodedString;
                                    bBase64Decode = 0;
                                
                                }*/
                                pData->ppszFieldValue[tCurIndex] = pszFieldValue;
                                tCurIndex++;
                                tTotalNoOfFields++;
                                
                            }
                            tLineStarted = i;
                        }
                        bLineSeparator = 0;
                        break;
            }
            if (bFieldEnded == 1)
            {
               goto cleanup;
            }
            tFieldLength++;
        }
        /*Go back and posize_t to the start of last unfinished line*/
        
        if (tTotalNoOfFields == 0)
        {
            printf("\n Some problem");
            dwError = LW_ERROR_CSV_BAD_FORMAT;
            BAIL(dwError);
        }
        
        bLineFeed = 0; //Start of the next LIne
        bLineSeparator = 0;

        pLdif->tPosition = pLdif->tPosition+tLineEnded;
        lseek(pLdif->fd, pLdif->tPosition, SEEK_SET);
        tLineStarted = 0; 
        tLineEnded = 0;
        tFieldValueStarted = 0;
    }

cleanup:
    *ppData = pData;
    pData->nFields = pLdif->nFields;    
    pLdif->tPosition += tFieldLength -1;
    return dwError;

error:
    if (pszFieldValue)
    {
        free(pszFieldValue);
        pszFieldValue = NULL;
    }
    if (pszFieldName)
    {
        free(pszFieldName);
        pszFieldName = NULL;
    }
    Lwt_LsaTestFreeData(pData);
    
    goto cleanup;
}



/*
 * Frees the data pointed to by PLWTLDIF
 * Frees the file
 * 
 */
static
DWORD
LdifFree(
    PVOID pvLdif
    )
{
    PLWTLDIF pLdif = (PLWTLDIF) pvLdif;
    if(pLdif)
    {
        close(pLdif->fd);
        pLdif->fd = -1;
        free(pLdif);
        pLdif = NULL;
    }
    return LW_ERROR_SUCCESS;
}


DWORD 
LdifReset(
        PVOID pvLdif)
{
    PLWTLDIF pLdif = (PLWTLDIF) pvLdif;
    lseek(pLdif->fd, 0, SEEK_SET);
    return LW_ERROR_SUCCESS;
}

static 
DWORD 
CopyBuffer(
    PSTR pszBuf, 
    PSTR *ppStrValue, 
    size_t tLength)
{
    PSTR pszFieldValue = NULL;
    DWORD dwError = LW_ERROR_SUCCESS;

    pszFieldValue = malloc(tLength+1);
    if (pszFieldValue  == NULL)
    {
       return  LW_ERROR_OUT_OF_MEMORY;
    }
    memcpy(pszFieldValue, pszBuf, tLength);
    pszFieldValue[tLength] = '\0';
    *ppStrValue = pszFieldValue;
  
    return dwError;

}
