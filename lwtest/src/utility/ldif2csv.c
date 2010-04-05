
/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */



/*
 * Module Name: ldif2csv.c
 *
 * Converts the LDIF file information in to the CSV file info
 * 
 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
//#include <unistd.h>
#include <sys/stat.h>


#define LW_ERROR_BAD_FORMAT 1
#define LW_ERROR_OUT_OF_MEMORY 2
#define LW_ERROR_SUCCESS 0
#define LW_ERROR_FIELD_NOT_FOUND 5


#define DWORD int
#define PDWORD int *

#define BOOL int

#define BYTE unsigned char

#define BAIL(dwError) \
    if(dwError) \
    {\
        goto error;\
    }\


typedef char* PSTR;


typedef struct _LWT_DATA
{
    PSTR *ppszFieldName;
    PSTR *ppszFieldValue;
    int nFields;
}LWTDATA, *PLWTDATA;



typedef struct _CONFIG
{
    PSTR pszServerName;
    PSTR pszOutCsvName;
    PSTR *ppszDN;
    int nDCCount;
    PSTR pszOUName;
    PSTR pszFilterCategory;
    PSTR *ppszFieldName;
    int nFieldCount;
}CONFIG, *PCONFIG;






static
DWORD
Base64Decode(
    PSTR  pszInput,
    PSTR  *ppOctetString,
    PDWORD pdwByteCount
    );





static
DWORD
CsvWriteRecord(
        FILE *fd,
        PLWTDATA pLwtData,
        PSTR *ppszCsvField,
        int nMaxCsvFields
        );





static
DWORD
LdifCountNoOfFieldsnEntries(
            FILE *fd,
            size_t *nMaxFieldCount,
            size_t *nMaxEntries,
            size_t nOption
            );




DWORD
GetLen(
    PSTR pszField
    );




DWORD
CsvWriteHeader(
        FILE  *fd,
        PSTR *ppszCsvField,
        int  nMaxCsvFields
        );



DWORD
ParseArgs(
        int argc,
        char **argv,
        PCONFIG *ppConfig
        );




static
DWORD
CopyDN(
    PSTR pszArg, 
    PSTR **pppDN,
    int *nDCCount);





static
DWORD
CountNoOfFields(
            PSTR pszArg,
            BYTE delimiter,
            int *nSplitCount
            );





static 
DWORD
LdifGetNext(
    FILE *fd,
    size_t nIndex,
    PLWTDATA *ppData
    );


static
void
Lwt_LsaTestFreeData(
        PLWTDATA pData
        );


static
DWORD
CopyList(
        PSTR pszArg,
        PSTR **pppValue,
        int *nFieldCount);

void
ShowUsage();


static 
DWORD
ConfigFree(
        PCONFIG pConfig
          );

static 
DWORD 
CopyBuffer(
    PSTR pszBuf, 
    PSTR *ppStrValue, 
    size_t tLength);

static
DWORD
LwtDataGetGroupMembers(
                PLWTDATA pData, 
                PSTR *ppszFieldValue
                );


 PSTR ppszCsvField[]= {"sAMAccountName",
                        "dn",
                        "name",
                       "distinguishedName",
                        "uidNumber",
                        "gidNumber",
                        "loginShell",
                         "gecos",
                         "backlink"};



PSTR ppszGroupField[]={"sAMAccountName",
                       "dn",
                       "name",
                       "distinguishedName",
                       "objectSid",
                       "gidNumber",
                       "member"};
                        
                         

static
DWORD
CsvGetValue(
        PSTR pszFieldName,
        PLWTDATA pData,
        PSTR *ppszFieldValue
        );

static
DWORD
LsaDataLWCellsGetData(
                PSTR pszFieldName, 
                PSTR pszFieldData, 
                PSTR *ppszFieldValue
                );
                        
        


int 
main(
    int argc, 
    char **argv)
{
    

    PSTR pszLdifFile = "ldfexport.ldf";
    PSTR pszOutPutCsv = NULL;
    
    FILE *pCsvFd = NULL;
    FILE *ldiffd = 0;
    int nIndex = 0;
    size_t tTotalNoOfFields = 0, tTotalEntries = 0;
    int nMaxCsvFields = 9 , i = 0;
    int nMaxGroupCsvField = 7;
 
    DWORD dwError = 0;

    PSTR *ppszFieldNames = NULL;

    PLWTDATA pLwtData = NULL;
    PCONFIG pConfig = NULL;
    char command[256] = {0};
    char *Quotes="\"";
    int nType;
    
    if(argc < 2 || argc > 9)
    {
        printf("Invalid argument \n\n"); 
        ShowUsage();
        return 0;
    }


    dwError = ParseArgs(argc-1, &argv[1], &pConfig);
    if(dwError)
    {
        goto error;
    }
    
    if (pConfig->pszServerName == NULL)
    {
        printf("\n Please specify the server name\n\n\n");
        ShowUsage();
    }
    snprintf(command, sizeof(command), "ldifde -f %s -s %s ", pszLdifFile,  pConfig->pszServerName);
    
    if (pConfig->nDCCount > 0)
    {
        strcat(command, "-d ");
        strcat(command, Quotes);
        
        for (i = 0; i < pConfig->nDCCount; i++)
        {
            strcat(command, "DC=");

            strcat(command, pConfig->ppszDN[i]);

            if ( i != pConfig->nDCCount -1 )
                strcat(command, ",");
        }
        strcat(command, Quotes);
    }
    
    strcat(command, " -r ");
    strcat(command,"(objectclass=");

    if (pConfig->pszFilterCategory)
    {
        strcat(command, pConfig->pszFilterCategory);
        strcat(command, ") ");
        
        if (!strcmp(pConfig->pszFilterCategory, "group"))
        {
            nType = 1;
        }
        else
        {
            nType = 0;
        }
 
    }
    else
    {
        strcat(command, "user) ");
        nType = 0;
    }
    
    if (pConfig->ppszFieldName)
    {
        strcat(command, "-l ");
        strcat(command, Quotes);
        for (i = 0; i < pConfig->nFieldCount; i++)
        {
            strcat(command, pConfig->ppszFieldName[i]);
            
            if ( i != pConfig->nFieldCount -1 )
                strcat(command, ",");
        }
        strcat(command, Quotes);
    }
    //printf("\n %s \n", command);



#if WIN32 
    system(command);
#endif

#if 0
    printf("\n Parsed parameters");
    printf("\n Server Name: %s", pConfig->pszServerName);
    
    printf("\n Domain Name:" );


    for(i = 0; pConfig->ppszDN[i] != NULL; i++)
    {
        printf("\n\t\t\t %s", pConfig->ppszDN[i]);
    }
    
    printf("\n OU Name: %s", pConfig->pszOUName);
    printf("\n Filter category: %s",pConfig->pszFilterCategory);

    printf("\n Listed fileds :");
    for (i=0; pConfig->ppszFieldName[i] != NULL; i++)
    {
        printf("\n \t\t\t %s", pConfig->ppszFieldName[i]);
    }
#endif


    ldiffd = fopen(pszLdifFile, "r");
    if (ldiffd == NULL)
    {
        fprintf(stdout, "Unable to open input file %s", pszLdifFile);
        return 1;
    }
    if (pConfig->pszOutCsvName == NULL)
    {
        printf("\n please specify the output csv file name\n\n");
        ShowUsage();
    }
    
     
    pCsvFd = fopen(pConfig->pszOutCsvName, "w");
    if (pCsvFd == NULL)
    {
        fprintf(stdout, "Unable to open input file %s", pConfig->pszOutCsvName);
        return 1;
    }
    dwError = LdifCountNoOfFieldsnEntries(ldiffd, &tTotalNoOfFields, &tTotalEntries, 0);
    if(dwError)
    {
        goto error;
    }



    while (nIndex != tTotalEntries)
    {
        PLWTDATA pLwtData = NULL;
        dwError = LdifGetNext(ldiffd, nIndex, &pLwtData);
        if (dwError)
        {
            goto error;
        }
        if (nIndex == 0)
        {
            if (nType == 0)
            {

                dwError = CsvWriteHeader(pCsvFd, ppszCsvField, nMaxCsvFields);
                if (dwError)
                {
                    goto error;
                }
            }
            else
            {
                dwError = CsvWriteHeader(pCsvFd, ppszGroupField, nMaxGroupCsvField);
                if (dwError)
                {
                    goto error;
                }
            
            }
        }
        if (nType == 0)
        {
            dwError = CsvWriteRecord(pCsvFd, pLwtData, ppszCsvField,  nMaxCsvFields);
            if (dwError)
            {
                goto error;
            }
            Lwt_LsaTestFreeData(pLwtData);
        }
        else
        {
            dwError = CsvWriteRecord(pCsvFd, pLwtData, ppszGroupField,  nMaxGroupCsvField);
            if (dwError)
            {
                goto error;
            }
            Lwt_LsaTestFreeData(pLwtData);
        }
        nIndex++;
    }

cleanup :
    fflush(pCsvFd);
    if (ldiffd)
    {

        fclose(ldiffd);
    }
    if (pCsvFd)
    {
        fclose(pCsvFd);
    }
    /*
    if(ppCsvFields != NULL)
    {
        for( i = 0; i < nMaxCsvFields; i++)
        {
            free(ppCsvFields[i]);
        }
        free(ppCsv);
    }*/

    return 0; 


error :
    goto cleanup;
}





DWORD
ParseArgs(
        int argc,
        char **argv,
        PCONFIG *ppConfig
        )
{


    int nIndex = 0;
    PSTR pszArg = NULL;
    DWORD dwError = LW_ERROR_SUCCESS;
    PCONFIG pConfig = NULL;
    size_t tLen = 0;
    
    typedef enum{

            PARSE_MODE_OPEN = 0,
            PARSE_MODE_SERVER,
            PARSE_MODE_FILTER,
            PARSE_MODE_DN,
            PARSE_MODE_LIST,
            PARSE_MODE_OUTFILENAME
            } ParseMode;
        
    ParseMode nParseMode = PARSE_MODE_OPEN;



    if(argc <= 1 || argv == NULL)
    {
        printf("\n\nInvalid argument\n\n");
        ShowUsage();
        return LW_ERROR_BAD_FORMAT;
    }
    pConfig = malloc(sizeof(CONFIG));
    
    do
    {
        pszArg = argv[nIndex]; 

        switch (nParseMode)
        {
            case PARSE_MODE_OPEN :
                {
                    if (!strcmp(pszArg, "-s"))
                    {
                        nParseMode = PARSE_MODE_SERVER; 
                    }
                    else if (!strcmp(pszArg, "-d"))
                    {
                        nParseMode = PARSE_MODE_DN;
                    }
                    else if (!strcmp(pszArg, "-f"))
                    {
                        nParseMode = PARSE_MODE_FILTER;
                    }
                    else if(!strcmp(pszArg, "-l"))
                    {
                        nParseMode = PARSE_MODE_LIST;
                    }
                    else if(!strcmp(pszArg, "-o"))
                    {
                        nParseMode = PARSE_MODE_OUTFILENAME;
                    }
                    else
                    {
                        dwError = LW_ERROR_BAD_FORMAT;
                        BAIL(dwError);
                    }

               }
               break;
        case PARSE_MODE_SERVER : 
                {
                    size_t tLen = 0;
                    tLen = strlen(pszArg);
                    CopyBuffer(pszArg, &pConfig->pszServerName, tLen); 
                    nParseMode = PARSE_MODE_OPEN;
                    break;
                
                }
                break;
        
        case PARSE_MODE_FILTER :
                {
                    size_t tLen = 0;
                    tLen = strlen(pszArg);
                    CopyBuffer(pszArg, &pConfig->pszFilterCategory, tLen);
                    nParseMode = PARSE_MODE_OPEN;

                }
                break;
        case PARSE_MODE_DN :

                    dwError = CopyDN(pszArg, &pConfig->ppszDN, &pConfig->nDCCount);
                    BAIL(dwError);
                    nParseMode = PARSE_MODE_OPEN;
                    break;

        case PARSE_MODE_LIST :
                dwError = CopyList(pszArg, &pConfig->ppszFieldName, &pConfig->nFieldCount);
                BAIL(dwError);
                nParseMode = PARSE_MODE_OPEN;
                break;
        case PARSE_MODE_OUTFILENAME :
                tLen = strlen(pszArg);
                pConfig->pszOutCsvName = malloc(tLen+1);
                memcpy(pConfig->pszOutCsvName, pszArg, tLen);
                pConfig->pszOutCsvName[tLen] = '\0';
                nParseMode = PARSE_MODE_OPEN;
                break;
        }
        nIndex++;
    }while(nIndex != argc);

cleanup:
    *ppConfig = pConfig;
    return  dwError;

error :
    ConfigFree(pConfig);
    goto cleanup;
        

 }


/*
 *
 * Copy the DC in the config structure
 *
 *
 *
 */

static
DWORD
CopyDN(
    PSTR pszArg, 
    PSTR **pppDN,
    int *nDCCount)
{

    PSTR *ppFields = NULL;
    int nCount = 0;
    size_t tLength = 0, tValueStarted = 0;
    size_t tFieldCount = 0;
    PSTR pszValue = NULL;
    DWORD dwError = LW_ERROR_SUCCESS;
    
    size_t i = 0, tValueStart = 0;;

    dwError = CountNoOfFields(pszArg, '.', &nCount);
    BAIL(dwError);
    

    ppFields = calloc(nCount, sizeof(PSTR));
    if (ppFields == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        return;
    }
    
    do
    {
        
        if(pszArg[i] == '.' || pszArg[i] == '\0')
        {
            tLength = i - tValueStart;
            CopyBuffer((pszArg+tValueStart), &pszValue, tLength);
            ppFields[tFieldCount++] = pszValue;
            tValueStart = i + 1;
        }
        i++;
    }while(tFieldCount != nCount);    

cleanup :
    *pppDN = ppFields;
    *nDCCount = nCount;
    return dwError;

error :
    for(i =0; i < nCount; i++)
    {
        if(ppFields[i] != NULL)
        {
            free(ppFields[i]);
            ppFields[i] = NULL;
        }
    }
    if(ppFields)
    {
        free(ppFields);
        ppFields = NULL;
    }
    goto cleanup;

}
    


/*
 *
 * Parser utilities
 *
 */

static
DWORD
CountNoOfFields(
            PSTR pszArg,
            BYTE delimiter,
            int *nSplitCount)
{

    int nFieldCount = 0;
    size_t tIndex = 0;
    DWORD dwError = LW_ERROR_SUCCESS;

    while(pszArg[tIndex] != '\0')
    {
        if(pszArg[tIndex] == delimiter)
        {
            nFieldCount++;
        }
        tIndex++;
    }

    *nSplitCount = nFieldCount + 1;
    return dwError;
}



/*
 *
 * Copies the filtering values based on the list
 *
 *
 *
 */

static
DWORD
CopyList(
        PSTR pszArg,
        PSTR **pppValue,
        int *nFieldCount)
{
    PSTR *ppValue = NULL;
    int i = 0;
    size_t tValueStart = 0, tLength = 0;
    PSTR pszValue = NULL;
    DWORD dwError = LW_ERROR_SUCCESS;
    size_t tFieldCount = 0;
    size_t tIndex = 0;
    
    dwError = CountNoOfFields(pszArg, ',', &tFieldCount);
    
    ppValue = calloc(tFieldCount, sizeof(PSTR));
    if(ppValue == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL(dwError);
    }
    

    do
    {
        
        if(pszArg[i] == ',' || pszArg[i] == '\0')
        {
            tLength = i - tValueStart;
            CopyBuffer(pszArg+tValueStart, &pszValue, tLength);
            ppValue[tIndex++] = pszValue;
            tValueStart = i + 1;
        }

        i++;
    }while(tIndex != tFieldCount);    

cleanup :
    *pppValue = ppValue;
    *nFieldCount = tFieldCount;
    return dwError;


error :
    for(i =0; i < tFieldCount; i++)
    {
        if(ppValue[i] != NULL)
        {
            free(ppValue[i]);
            ppValue[i] = NULL;
        }
    }
    if(ppValue)
    {
        free(ppValue);
        ppValue = NULL;
    }
    goto cleanup;

}  



/*
 *
 * LDIF parser code
 *
 *
 */
 
static 
DWORD
LdifGetNext(
    FILE *fd,
    size_t nIndex,
    PLWTDATA *ppData
    )

{

    char pszBuf[512];
    size_t tLineStarted = 0, tLineEnded = 0;
    BOOL bLineFeed = 0, bFieldEnded = 0, bLineSeparator= 0;
    BOOL bBase64Decode = 0;
    size_t tCurIndex = 0;
    size_t tFieldValueStarted = 0;
    size_t tTotalNoOfFields = 0, tTotalEntries = -1;
    size_t tFieldNameEnded = 0, tFieldLength = 0;
    DWORD dwError = LW_ERROR_SUCCESS;
    int nBufCount = 0, i = 0;
    PLWTDATA pData = NULL;
    size_t tLength;
    PSTR pszFieldName = NULL;
    PSTR pszFieldValue = NULL;
    PSTR pszDecodedString = NULL;
    static int tPosition = 0;



    //Reset
    fseek(fd, tPosition, SEEK_SET);
    
    /* Count the no of fields in this block*/
    dwError = LdifCountNoOfFieldsnEntries(fd, &tTotalNoOfFields, &tTotalEntries , tPosition);
    if (dwError)
    {
        goto error;
    }

    fseek(fd, tPosition, SEEK_SET);
    //Copy the data in to pData;
    pData = malloc(sizeof(PLWTDATA));
    if (pData == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL(dwError);
    }
    pData->ppszFieldName = calloc(tTotalNoOfFields, sizeof(char **));
    if(pData->ppszFieldName == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL(dwError);
    }
    
    pData->ppszFieldValue = calloc(tTotalNoOfFields, sizeof(char **));
    if(pData->ppszFieldValue == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL(dwError);
    }
   
    tTotalNoOfFields = 0; 
    /*Go back to the start of the position before counting */
    while ((nBufCount = fread(pszBuf, 1,  sizeof(pszBuf), fd)) > 0)
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
                                BAIL(dwError);
                                pData->ppszFieldName[tCurIndex] = pszFieldName; 
                            
                                tLength = tLineEnded - tFieldValueStarted;
                                dwError = CopyBuffer((pszBuf+tFieldValueStarted), &pszFieldValue, tLength);
                                BAIL(dwError);


                                if (bBase64Decode)
                                {
                                    DWORD dwKeyLength = 0;
                                    dwError = Base64Decode(pszFieldValue, &pszDecodedString, &dwKeyLength);
                                    BAIL(dwError);

                                    free(pszFieldValue);
                                    pszFieldValue = pszDecodedString;
                                    bBase64Decode = 0;
                                
                                }
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
                            dwError = LW_ERROR_BAD_FORMAT;
                            BAIL(dwError);
                        }
                        break;
                case ':' :
                        if (i == tFieldNameEnded + 1) 
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
                            
                            //If the line is started just after the fieldName, recalculate field Start
                            if (i == tFieldValueStarted + 2)
                            {
                                tFieldValueStarted = i+1;
                            } 
                            
                        }
                        break;

                default :
                        if (bLineSeparator == 1)
                        {
                            if (tLineEnded != i - 2)
                            {
                                /* Copy the field from start of the line to end */
                                tLineEnded = i -2;
                                tLength = tFieldNameEnded - tLineStarted;
                                dwError = CopyBuffer((pszBuf+tLineStarted), &pszFieldName, tLength);
                                BAIL(dwError);
                                pData->ppszFieldName[tCurIndex] = pszFieldName;
                        
                            
                                tLength = tLineEnded - tFieldValueStarted;
                                dwError = CopyBuffer((pszBuf+tFieldValueStarted), &pszFieldValue, tLength);
                                BAIL(dwError);
    
                                if (bBase64Decode)
                                {
                                    DWORD dwKeyLength = 0;
                                    dwError = Base64Decode(pszFieldValue, &pszDecodedString, &dwKeyLength);
                                    BAIL(dwError);
                                    free(pszFieldValue);
                                    pszFieldValue = pszDecodedString;
                                    bBase64Decode = 0;
                                
                                }
                                pData->ppszFieldValue[tCurIndex] = pszFieldValue;
                                tCurIndex++;
                                tTotalNoOfFields++;
                                tLineEnded = i-2;
                                
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
            dwError = LW_ERROR_BAD_FORMAT;
            BAIL(dwError);
        }
        
        bLineFeed = 0; //Start of the next LIne

        tPosition += tLineEnded;
        fseek(fd, tPosition, SEEK_SET);
        tLineStarted = 0; 
        tLineEnded = 0;
        tFieldValueStarted = 0;
    }

cleanup:
    *ppData = pData;
    pData->nFields = tTotalNoOfFields;    
    tPosition += tFieldLength -1;
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




static
void
Lwt_LsaTestFreeData(
        PLWTDATA pData
        )
{
    size_t i;
    for( i = 0; i < pData->nFields; i++)
    {
        if(pData->ppszFieldValue[i])
        {
            free(pData->ppszFieldValue[i]);
            pData->ppszFieldValue[i] = NULL;
        }
        if(pData->ppszFieldName[i])
        {
            free(pData->ppszFieldName[i]);
            pData->ppszFieldName[i] = NULL;
        }
    }
    free(pData->ppszFieldValue);
    free(pData->ppszFieldName);
    
    pData->ppszFieldName = NULL;    
    pData->ppszFieldValue = NULL;

    free(pData);
}



/*
 *
 * Writes the records in to the csv file
 *
 *
 */

DWORD
CsvWriteRecord(
        FILE *fd,
        PLWTDATA pLwtData,
        PSTR *ppszCsvField,
        int nMaxCsvFields
        )
{
    int nFields = pLwtData->nFields;
    PSTR pszFieldName = NULL;
    PSTR pszFieldValue = NULL;
    DWORD dwError = LW_ERROR_SUCCESS;
    char quote = '"';

    int i = 0; 

    for(i = 0; i < nMaxCsvFields; i++)
    {
        dwError = CsvGetValue(ppszCsvField[i], pLwtData, &pszFieldValue);
        if (dwError)
        {
            fprintf(fd, ",");
        }
        else
        {
            //For multi fleid valuesClose
            if( !strcmp(ppszCsvField[i],  "dn") || !strcmp(ppszCsvField[i], "distinguishedName"))
            {
                fprintf(fd, "%c", quote);
                
            }
            
            fprintf(fd, "%s", pszFieldValue);
            free (pszFieldValue);
            
            //For multi field values
            if( !strcmp(ppszCsvField[i], "dn") || !strcmp(ppszCsvField[i], "distinguishedName"))
            {
                fprintf(fd, "%c", quote);
            }
            fprintf(fd, ",");
        }
    }
    fprintf(fd, "\n");
    if (dwError == LW_ERROR_FIELD_NOT_FOUND)
    {
        return LW_ERROR_SUCCESS;
    }
    return dwError;
}   


/*
 *
 * First Line header wrting for csv
 *
 */

DWORD
CsvWriteHeader(
        FILE *fd,
        PSTR *ppszCsvField,
        int  nMaxCsvFields
        )
{

    int i = 0;
    DWORD dwError = LW_ERROR_SUCCESS;

    for( i = 0; i < nMaxCsvFields; i++)
    {
        if(!strcmp("backLink", ppszCsvField[i]))
        {
            fprintf(fd,"Sid,");
        }
        else
        {
            fprintf(fd,"%s,", ppszCsvField[i]);
        }
    }
    fprintf(fd, "lastfield\n");
    return dwError;
}





/*
 *
 * count the no of the fields in the LDIF structrue
 *
 *
 */


static
DWORD
LdifCountNoOfFieldsnEntries(
            FILE  *fd,
            size_t *nMaxFieldCount,
            size_t *nMaxEntries,
            size_t tPosition 
            )
{

    char pszBuf[512];
    BOOL bFieldEnded = 0, bLineSeparator = 0, bLineFeed = 0;
    BOOL bFieldNameEnded = 0;
    DWORD dwError = LW_ERROR_SUCCESS;
    int nBufCount = 0;
    size_t tNumberOfField = 0;
    size_t tNumberOfEntries = 0, tSeekPosition = 0 ;
    size_t tLineEnded = 0, i = 0;



    /* Variable lenght fields..Make sure you count them right for initial entire reading, then field by field counts*/
    fseek(fd, tPosition, SEEK_SET);    


    /*TODO: revisit this code and review it*/
    while ((nBufCount = fread(pszBuf, 1, sizeof(pszBuf), fd)) > 0)
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
                            dwError = LW_ERROR_BAD_FORMAT;
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
                    if (*nMaxEntries == -1)
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
            fseek(fd, tSeekPosition+tPosition, SEEK_SET);
            tLineEnded = 0;
        }
    }
cleanup:
    *nMaxEntries = tNumberOfEntries;
    return dwError; 
error:
    goto cleanup;
}



/*
 *
 * free CONFIG structure.
 *
 */

static 
DWORD
ConfigFree(
        PCONFIG pConfig
          )
{
    int i = 0;
    
    if (pConfig->pszServerName)
    {
        free(pConfig->pszServerName); 
        pConfig->pszServerName = NULL;
    }

    if (pConfig->pszFilterCategory)
    {
        free(pConfig->pszFilterCategory);
        pConfig->pszFilterCategory = NULL;

    }
    if (pConfig->pszOutCsvName)
    {
        free(pConfig->pszOutCsvName);
        pConfig->pszOutCsvName = NULL;

    }

    if (pConfig->ppszDN)
    {
        for (i = 0; pConfig->ppszDN[i] != NULL; i++)
        {
            free(pConfig->ppszDN[i]);
            pConfig->ppszDN[i] = NULL;
        }
        free(pConfig->ppszDN);
        pConfig->ppszDN = NULL;
    }
    
    if (pConfig->ppszFieldName)
    {
        for (i = 0; pConfig->ppszFieldName[i] != NULL; i++)
        {
            free(pConfig->ppszFieldName[i]);
            pConfig->ppszFieldName[i] = NULL;
        }
        free(pConfig->ppszFieldName);
        pConfig->ppszFieldName = NULL;
    }
   
}




void
ShowUsage()
{
    printf("\n Usage : ldif2csv [options] \n");
    printf( "\n-s <servername> \n [ -o <outfile.csv> ][-f  <user/groups>] [-l \"<field1> <field2>...<fieldn>\"] [-d <DCNAME>] \n");        
}




/*
 * utiltiy api to copy the buffer
 *
 */

static 
DWORD 
CopyBuffer(
    PSTR pszBuf, 
    PSTR *ppStrValue, 
    size_t tLength)
{
    PSTR pszFieldValue = NULL;
    DWORD dwError = LW_ERROR_SUCCESS;
    size_t tStrLength = 0;
    size_t i = 0;
    size_t j = 0;
    
    for (i =0; i < tLength; i++)
    {
        if(pszBuf[i] != '\r' && pszBuf[i] != '\n')
        {
            tStrLength++;
        }
    }

    
    pszFieldValue = malloc(tStrLength+1);
    if (pszFieldValue  == NULL)
    {
       return  LW_ERROR_OUT_OF_MEMORY;
    }
    
    for (i =0; i < tLength; i++)
    {
        if(pszBuf[i] != '\r' &&  pszBuf[i] != '\n')
        {
            //Dont copy the initial space if it is folderd line.
            pszFieldValue[j++] = pszBuf[i]; 
        }
    }
    pszFieldValue[tStrLength] = '\0';
    *ppStrValue = pszFieldValue;
  
    return dwError;
}



/*
 *
 * Utilities for the Base 64 decoding stuff
 *
 */
static const unsigned char Base64Table[64] = 
{
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', '+', '/'
};

unsigned char bits(int c)
{
    if ( 'A' <= c && c <= 'Z' )
        return c - 'A';             /* 0 .. 25 */
    else if ( 'a' <= c && c <= 'z' )
        return 26 + (c - 'a');      /* 26 .. 51 */
    else if ( '0' <= c && c <= '9' )
        return 52 + (c - '0');      /* 52 .. 61 */
    else if ( c == '+' )
        return 62;                  /* 62 */
    else if ( c == '/' )
        return 63;                  /* 63 */
    else
        return 255;                 /* Shouldn't get here! */
}




/*
 * Base 64 decode
 *
 */

static
DWORD
Base64Decode(
    PSTR  pszInput,
    PSTR  *ppOctetString,
    PDWORD pdwByteCount
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    PSTR pOctetString = NULL;
    DWORD dwByteCount = 0;

    char buf[4] = { 0 };
    PSTR pszStr = pszInput;

    DWORD dwBase64CharacterCount = 0;

    int c;
    int i, j;

    while ( (c = *pszStr) != '\0' )
    {
        if ( ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || (c >= '0' || c <= '9') ||
             c == '+' || c == '/' )
        {
            dwBase64CharacterCount++;
        }
        pszStr++;
    }

    /* 6 bits per characeter, 8 bits per byte */
    dwByteCount = (dwBase64CharacterCount * 6)/ 8;

    pOctetString = calloc(dwByteCount, 1);
    if ( ! pOctetString )
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        goto error;
    }
    

    pszStr = pszInput;
    i = 0;
    j = 0;
    while ( (c = *pszStr) != '\0' )
    {
        if ( ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || c == '+' ||c == '/' || c >= '0' || c <= '9' )
        {
            buf[i++] = c;
       
            if ( i == 4 )
            {
                pOctetString[j++] = 
                    ((bits(buf[0]) & 0x3f) << 2) | ((bits(buf[1]) & 0x30) >> 4);
                pOctetString[j++] = 
                    ((bits(buf[1]) & 0x0f) << 4) | ((bits(buf[2]) & 0x3c) >> 2);
                pOctetString[j++] = 
                    ((bits(buf[2]) & 0x03) << 6) | ((bits(buf[3]) & 0x3f) >> 0);

                buf[0] = buf[1] = buf[2] = buf[3] = '\0';
                i = 0;
            }
        }
        pszStr++;
    }
    while ( i < 4 )
        buf[i++] = 'A';


    if ( j < dwByteCount )
    {
        pOctetString[j++] = 
            ((bits(buf[0]) & 0x3f) << 2) | ((bits(buf[1]) & 0x30) >> 4);
    }
    if ( j < dwByteCount )
    {
        pOctetString[j++] = 
            ((bits(buf[1]) & 0x0f) << 4) | ((bits(buf[2]) & 0x3c) >> 2);
    }
    if ( j < dwByteCount )
    {
        pOctetString[j++] = 
            ((bits(buf[2]) & 0x03) << 6) | ((bits(buf[3]) & 0x3f) >> 0);
    }

cleanup:

    *pdwByteCount = dwByteCount;
    *ppOctetString = pOctetString;

    return dwError;

error:

    free(pOctetString);
    pOctetString = NULL;

    dwByteCount = 0;

    goto cleanup;
}




/*
 * 
 * Get the specific value from the data to be written
 * to the csv file
 *
 *
 */

static
DWORD
CsvGetValue(
        PSTR pszFieldName,
        PLWTDATA pData,
        PSTR *ppszFieldValue
        )
{

    PSTR pszFieldValue = NULL;
    PSTR pszFieldData = NULL;
    size_t tLen = 0;
    size_t i;
    DWORD dwError = LW_ERROR_SUCCESS;
    
    for (i =0; i < pData->nFields; i++)
    {
        if (!strcmp(pszFieldName, "member"))
        {
            dwError = LwtDataGetGroupMembers(pData, &pszFieldValue);
            
            if (dwError == LW_ERROR_SUCCESS)
            {
                *ppszFieldValue = pszFieldValue;
            
            }
            return dwError;
        }
        if(!strcmp(pszFieldName, pData->ppszFieldName[i]))
        {
            
            if (pData->ppszFieldValue[i] != NULL)
            {
                tLen = strlen(pData->ppszFieldValue[i]);
                pszFieldValue = malloc(tLen+1);
                memcpy(pszFieldValue, pData->ppszFieldValue[i], tLen);
                pszFieldValue[tLen] = '\0';
                *ppszFieldValue = pszFieldValue;
                return dwError;
            }
            else
            {
                //No Value in the field!!
                return LW_ERROR_FIELD_NOT_FOUND;
            }
                
        }
    }

    
    for (i =0; i < pData->nFields; i++)
    {
        pszFieldData = pData->ppszFieldValue[i];
    
        dwError = LsaDataLWCellsGetData(pszFieldName, pszFieldData, &pszFieldValue);
        if (dwError == LW_ERROR_SUCCESS)
        {
           *ppszFieldValue = pszFieldValue;
            return dwError;
        }
    }


    return LW_ERROR_FIELD_NOT_FOUND;      
}
        
        
            

/*
 *
 * Utitlity to get the likewise cells 
 * parameter from the data fields
 *
 */
static
DWORD
LsaDataLWCellsGetData(
                PSTR pszFieldName, 
                PSTR pszFieldData, 
                PSTR *ppszFieldValue
                )
{

    PSTR pszFieldValue = NULL;
    size_t i =0, tLen = 0;
    size_t tKeyPosition = 0;
    
    tLen = strlen(pszFieldData);

    for (i = 0; i != tLen; i++)
    {
        if (pszFieldData[i] == '=')
        {
            tKeyPosition = i;
            break;
        }
    }

    //If there is no likewise cells
    if (tKeyPosition == 0)
    {
        return LW_ERROR_FIELD_NOT_FOUND;
    }


    //Check if the likewise parameter we are interested in
    if (!strncmp(pszFieldData, pszFieldName, strlen(pszFieldName)))
    {
         tLen = tLen - tKeyPosition ; 

        if (tLen > 0)
        {
            pszFieldValue = malloc(tLen);
            memcpy(pszFieldValue, (pszFieldData+ tKeyPosition + 1 ), tLen);
            pszFieldValue[tLen] = '\0';

            *ppszFieldValue = pszFieldValue;
            return LW_ERROR_SUCCESS;
        }
         
    }
    return LW_ERROR_FIELD_NOT_FOUND;
}





/*
 *
 * utility to get the group member information in a single line
 * to print correctly in csv file
 */
static
DWORD
LwtDataGetGroupMembers(
                PLWTDATA pData, 
                PSTR *ppszFieldValue
                )
{
    
    int i = 0;
    PSTR pszFieldValue;
    size_t tLen = 0;
    size_t tBufferCopied = 0;

    for (i = 0; i < pData->nFields; i++)
    {
        if(!strcmp(pData->ppszFieldName[i], "member"))
        {
           tLen += GetLen(pData->ppszFieldValue[i]); 
        }
    }
    
    if (tLen > 0)    
    {
       pszFieldValue= malloc(tLen);
    }
    else
    {
        return LW_ERROR_FIELD_NOT_FOUND; 
    }
    
    /* Concat and copy the field in to a single line buffer */
    for( i = 0; i < pData->nFields; i++)
    {
        if(!strcmp(pData->ppszFieldName[i], "member"))
        {
            PSTR pszField = pData->ppszFieldValue[i];            

            tLen = GetLen(pszField);
            if(tLen > 0)
            {
                memcpy(pszFieldValue + tBufferCopied, pszField, tLen);
            }
            tBufferCopied += tLen;
        }
    }
    if (tBufferCopied > 0)
    {
        pszFieldValue[tBufferCopied -1] = '\0';
    }
    *ppszFieldValue = pszFieldValue;
    return LW_ERROR_SUCCESS;
}





/*
 *
 *
 * utitlity api to retrive memebrs from member distinguished name
 *
 *
 * */
DWORD
GetLen(
    PSTR pszBuf
    )
{

    size_t tIndex = 0;
    
    while(pszBuf[tIndex] != '\0' )
    {
        if(pszBuf[tIndex] == ',')
        {
            break;
        }
        tIndex++;
    }
    return tIndex +1;
        
}

