// Exportusers.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Sddl.h>
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


#include "stdafx.h"
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
#define DEBUG 0

#define ADS_UF_ACCOUNTDISABLE 0x02
#define ADS_PASSWD_CANT_CHANGE 0x40
#define ADS_UF_DONT_EXPIRE_PASSWD 0x10000



//#define DWORD int
//#define PDWORD int *

#define BOOL int

#define BYTE unsigned char

#define BAIL(dwError) \
    if(dwError) \
    {\
        goto error;\
    }\


typedef char* PSTR;
PSTR pszFQDN = NULL;

typedef struct _LWT_DATA
{
    PSTR *ppszFieldName;
    PSTR *ppszFieldValue;
	//LPWSTR *pszBase64DecodedStrings
    int nFields;
}LWTDATA, *PLWTDATA;



typedef struct _CONFIG
{
    PSTR pszServerName;
    PSTR pszOutCsvName;
#if 0
    PSTR *ppszDN;
    int nDCCount;
    PSTR pszOUName;
    PSTR pszFilterCategory;
    PSTR *ppszFieldName;
    int nFieldCount;
#endif
}CONFIG, *PCONFIG;






static
DWORD
Base64Decode(
    PSTR  pszInput,
    PSTR  *ppOctetString,
    PDWORD pdwByteCount
    );

DWORD ConvertBase64toSecurityDescriptor(
                               PSTR pszEncodedString, 
                               PSTR *pszSid);



static
DWORD
CsvWriteRecord(
        FILE *fd,
        PLWTDATA pLwtData,
        PSTR *ppszCsvField,
        int nMaxCsvFields
        );
static
BOOL 
Get_PasswordNeverExpires(
						PSTR pszAcountEnabled
						);
static
BOOL 
Get_CanChangePassword(
					 PSTR pszAcountEnabled
					  );

static
BOOL
Get_AccountEnabled(
				  PSTR pszAcountEnabled
				  );


static
BOOL 
Get_AccountExpired(
				PSTR pszAccountExpiresValue
					);


static
DWORD
LdifCountNoOfFieldsnEntries(
            FILE *fd,
            int *nMaxFieldCount,
            int *nMaxEntries,
            int nOption
            );

char gpszNetBiosName[255];
static
BOOL
getNetBiosName(
			   PSTR pszDomainName
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




/*static
DWORD
CopyDN(
    PSTR pszArg, 
    PSTR **pppDN,
    int *nDCCount);*/

static
DWORD
CsvWriteField(
        FILE *fd,
        PSTR pszFieldValue
		);




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
    int nIndex,
    PLWTDATA pData
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
    int tLength);

static
DWORD
LwtDataGetGroupMembers(
                PLWTDATA pData, 
                PSTR *ppszFieldValue
                );


  

PSTR ppszCsvField[]={"NTName",
					 "SamAccountName",
					 "Sid",
					 "UserPrincipalName",
			         "FQDN",
			         "NetBiosName",
					 "DistinguishedName",
			         "name",
					 "st",
					 "givenName",
					 "sn",
					 "description",
					 "accountExpires",
					 "badPasswordTime",
					 "badPwdCount",
			         "UnixUid",
			         "UnixGid",
			         "UnixGecos",
			         "UnixLoginShell",
			         "UnixHomeDirectory",
					 "Password",
			         "pwdLastSet",
					 "canChangePassword",
			         "pwdNeverExpires",
			         "UserEnabled",
					 "PasswordExpirationDate",
			         "UserLockedOut",
					 "pwdExpires",
			         "lastField"};




                         

PSTR gpszFQDN = NULL;
static
DWORD
CsvGetField(
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
    int tTotalNoOfFields = 0, tTotalEntries = 0;
    int nMaxCsvFields = 29, i = 0;
    int nMaxGroupCsvField = 7;
 
    DWORD dwError = 0;

    PSTR *ppszFieldNames = NULL;

    
    PCONFIG pConfig = NULL;
    char command[512] = {0};
    char *Quotes="\"";
    int nType = 0;
    
    if(argc != 5)
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
		return 1;
    }
	sprintf_s(command, sizeof(command), "ldifde -f %s -s %s ", pszLdifFile, pConfig->pszServerName);
        
    strcat(command, " -r ");
    strcat(command,"(objectclass=user)");
    
	DWORD dwMaxLength = MAX_COMPUTERNAME_LENGTH + 1;
	
    if( getNetBiosName(pConfig->pszServerName))
    {
        printf("\n Problem in getting the netbios name");
        return 1;
    }
	gpszFQDN = pConfig->pszServerName;


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

	//printf("\n opening the ldf file", pszLdifFile);
    ldiffd = fopen(pszLdifFile, "rb");
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
    
     
    pCsvFd = fopen(pConfig->pszOutCsvName, "w+");
    if (pCsvFd == NULL)
    {
        fprintf(stdout, "Unable to open input file %s", pConfig->pszOutCsvName);
        return 1;
    }
	
    dwError = LdifCountNoOfFieldsnEntries(ldiffd, &tTotalNoOfFields, &tTotalEntries, 0);
    if(dwError)
    {

		printf("\n Error counting number of records in ldf file");
        goto error;
    }
	



    PLWTDATA pLwtData = NULL;
    //Copy the data in to pData;
    pLwtData = (PLWTDATA) malloc(sizeof(PLWTDATA));
    if (pLwtData == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
		goto error;
	}

	dwError = CsvWriteHeader(pCsvFd, ppszCsvField, nMaxCsvFields);
	if (dwError)
	{
		printf("\n Error in writing header to CSV");
		goto error;
	}
    while (nIndex != tTotalEntries)
    {
        dwError = LdifGetNext(ldiffd, nIndex, pLwtData);
        if (dwError)
        {
			printf("\n Error in getting data from ldf file");
            goto error;
        }
		dwError = CsvWriteRecord(pCsvFd, pLwtData, ppszCsvField,  nMaxCsvFields);
		if (dwError)
		{
			printf("\n Error in writing data to CSV");
			goto error;
		}
		Lwt_LsaTestFreeData(pLwtData);
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
	//free(pLwtData);
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
    int tLen = 0;
    
    typedef enum{

            PARSE_MODE_OPEN = 0,
            PARSE_MODE_SERVER,
            PARSE_MODE_OUTFILENAME
            } ParseMode;
        
    ParseMode nParseMode = PARSE_MODE_OPEN;



    if(argc <= 1 || argv == NULL)
    {
        printf("\n\nInvalid argument\n\n");
        ShowUsage();
        return LW_ERROR_BAD_FORMAT;
    }
    pConfig = (PCONFIG) malloc(sizeof(CONFIG));
	pConfig->pszOutCsvName = NULL;
	pConfig->pszServerName = NULL;

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
                    int tLen = 0;
                    tLen = strlen(pszArg);
                    CopyBuffer(pszArg, &pConfig->pszServerName, tLen); 
                    nParseMode = PARSE_MODE_OPEN;
                    break;
                
                }
                break;
        case PARSE_MODE_OUTFILENAME :
                tLen = strlen(pszArg);
                pConfig->pszOutCsvName = (PSTR ) malloc(tLen+1);
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
 

static
DWORD
CopyDN(
    PSTR pszArg, 
    PSTR **pppDN,
    int *nDCCount)
{

    PSTR *ppFields = NULL;
    int nCount = 0;
    int tLength = 0, tValueStarted = 0;
    int tFieldCount = 0;
    PSTR pszValue = NULL;
    DWORD dwError = LW_ERROR_SUCCESS;
    
    int i = 0, tValueStart = 0;;

    dwError = CountNoOfFields(pszArg, '.', &nCount);
    BAIL(dwError);
    

    ppFields = (PSTR *)calloc(nCount, sizeof(PSTR));
    if (ppFields == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        return dwError;
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

}*/
    


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
    int tIndex = 0;
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
    int tValueStart = 0, tLength = 0;
    PSTR pszValue = NULL;
    DWORD dwError = LW_ERROR_SUCCESS;
    int tFieldCount = 0;
    int tIndex = 0;
    
    dwError = CountNoOfFields(pszArg, ',', &tFieldCount);
    
    ppValue = (PSTR *) calloc(tFieldCount, sizeof(PSTR));
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
    int nIndex,
    PLWTDATA pData
    )

{

    char pszBuf[512];
    int tLineStarted = 0, tLineEnded = 0;
    BOOL bLineFeed = 0, bFieldEnded = 0, bLineSeparator= 0;
    BOOL bBase64Decode = 0;
    int tCurIndex = 0;
    int tFieldValueStarted = 0;
    int tTotalNoOfFields = 0, tTotalEntries = -1;
    int tFieldNameEnded = 0, tFieldLength = 0;
    DWORD dwError = LW_ERROR_SUCCESS;
    int nBufCount = 0, i = 0;
    //PLWTDATA pData = NULL;
    int tLength;
    PSTR pszFieldName = NULL;
    PSTR pszFieldValue = NULL;
    PSTR pszDecodedString = NULL;
    static int tPosition = 0;



    //Reset
    fseek(fd, tPosition, SEEK_SET);
#if 0
	printf("\n %d", tPosition);
#endif
    
    /* Count the no of fields in this block*/
    dwError = LdifCountNoOfFieldsnEntries(fd, &tTotalNoOfFields, &tTotalEntries , tPosition);
    if (dwError)
    {
        goto error;
    }

    fseek(fd, tPosition, SEEK_SET);
    
    pData->ppszFieldName = (PSTR *)calloc(tTotalNoOfFields, sizeof(char *));
    if(pData->ppszFieldName == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL(dwError);
    }
    
    pData->ppszFieldValue = (PSTR *)calloc(tTotalNoOfFields, sizeof(char *));
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
                                    dwError = ConvertBase64toSecurityDescriptor(pszFieldValue, &pszDecodedString);
                                    if(dwError == LW_ERROR_SUCCESS)
									{

										free(pszFieldValue);
										pszFieldValue = pszDecodedString;
										bBase64Decode = 0;
									}
                                
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
                                    dwError = ConvertBase64toSecurityDescriptor(pszFieldValue, &pszDecodedString);
                                    if(dwError == LW_ERROR_SUCCESS)
									{
										free(pszFieldValue);
										pszFieldValue = pszDecodedString;
										bBase64Decode = 0;
									}
                                
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
		bLineSeparator = 0;
        tPosition += tLineEnded;
        fseek(fd, tPosition, SEEK_SET);
        tLineStarted = 0; 
        tLineEnded = 0;
        tFieldValueStarted = 0;
    }

cleanup:
    //*ppData = pData;
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
    int i;
    for( i = 0; i < pData->nFields; i++)
    {
        if(pData->ppszFieldValue[i])
        {
#if 0
			printf("\n Freeing field value %s\n", pData->ppszFieldValue[i]);
#endif
            free(pData->ppszFieldValue[i]);
            pData->ppszFieldValue[i] = NULL;
        }
        if(pData->ppszFieldName[i])
        {
#if 0
			printf("\n Freeing field name %s\n", pData->ppszFieldName[i]);
#endif
            free(pData->ppszFieldName[i]);
            pData->ppszFieldName[i] = NULL;
        }
    }
    free(pData->ppszFieldValue);
    free(pData->ppszFieldName);
    
    pData->ppszFieldName = NULL;    
    pData->ppszFieldValue = NULL;

    
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
    
	PSTR pszNTName = NULL;
	DWORD dwLen = 0;
    

    //Write SAM Account Name
    CsvGetField("sAMAccountName", pLwtData,  &pszFieldValue);
	dwLen = strlen(pszFieldValue) + strlen(gpszNetBiosName);
	
	pszNTName = (PSTR )malloc(255);
	sprintf_s(pszNTName, 255, "%s\\%s", gpszNetBiosName, pszFieldValue);
    
	fprintf(fd, "%s,", pszNTName);

	//saMAccountName
    CsvWriteField(fd, pszFieldValue);

    

    //Write SID
    CsvGetField("objectSid", pLwtData, &pszFieldValue);
    CsvWriteField(fd, pszFieldValue);

    //Write Principal Name
    dwError = CsvGetField("userPrincipalName", pLwtData, &pszFieldValue);
	if(dwError != LW_ERROR_FIELD_NOT_FOUND)
	{
		CsvWriteField(fd, pszFieldValue);
	}
	else
	{
		fprintf(fd,",");
	}
	
    fprintf(fd,"%s,", gpszFQDN);

    //Write NetBiosName
	fprintf(fd,"%s,", gpszNetBiosName);
    
    //Write DN
    //For multi fleid valuesClose
    fprintf(fd, "%c", quote);
    CsvGetField("distinguishedName", pLwtData, &pszFieldValue);
    fprintf(fd, "%s", pszFieldValue);
    fprintf(fd, "%c,", quote);
	
    //Write name
    CsvGetField("name", pLwtData, &pszFieldValue);
    CsvWriteField(fd, pszFieldValue);
    
    //Write st
    CsvGetField("st", pLwtData, &pszFieldValue);
    CsvWriteField(fd, pszFieldValue);

    //Write givenName
    CsvGetField("givenName", pLwtData, &pszFieldValue);
    CsvWriteField(fd, pszFieldValue);
    
    //Write st
    CsvGetField("sn", pLwtData, &pszFieldValue);
    CsvWriteField(fd, pszFieldValue);
	

    //Write Description
    CsvGetField("description", pLwtData, &pszFieldValue);
    CsvWriteField(fd, pszFieldValue);

	//Get Account Expires
    CsvGetField("accountExpires", pLwtData, &pszFieldValue);
    CsvWriteField(fd, pszFieldValue);

	CsvGetField("badPasswordTime", pLwtData, &pszFieldValue);
    CsvWriteField(fd, pszFieldValue);

	CsvGetField("badPwdCount", pLwtData, &pszFieldValue);
    CsvWriteField(fd, pszFieldValue);


	
    //Write Uid Number
    CsvGetField("uidNumber", pLwtData,  &pszFieldValue);
    CsvWriteField(fd, pszFieldValue);
	
    //Write Gid Number
    CsvGetField("gidNumber", pLwtData, &pszFieldValue);
    CsvWriteField(fd, pszFieldValue);
	
    //Write Unix gecos
    CsvGetField("gecos", pLwtData, &pszFieldValue);
    CsvWriteField(fd, pszFieldValue);
	
    //Write Login Shell
    CsvGetField("shell", pLwtData, &pszFieldValue);
    CsvWriteField(fd, pszFieldValue);

	CsvGetField("UnixHomeDirectory", pLwtData, &pszFieldValue);
    CsvWriteField(fd, pszFieldValue);
	
    //Write last AD field
	
    //Write "Passwd"
    fprintf(fd, "password,");

	//Write Get Password Last set
    CsvGetField("pwdLastSet", pLwtData, &pszFieldValue);
    CsvWriteField(fd, pszFieldValue);

    
	//Write CanChange password
    CsvGetField("userAccountControl", pLwtData,  &pszFieldValue);
    dwError = Get_CanChangePassword(pszFieldValue);
    if (dwError != -1)
    {
        fprintf(fd, "%d",dwError);
    }
    fprintf(fd, ",");	
    //Get Password Expiration Date


    dwError = Get_PasswordNeverExpires(pszFieldValue);
    if (dwError != -1)
    {
        fprintf(fd, "%d",dwError);
    }
	fprintf(fd, ",");

    
    //Get Account Enabled
    dwError = Get_AccountEnabled(pszFieldValue);
    if(dwError != -1)
    {
        fprintf(fd, "%d",dwError);
    }
    fprintf(fd, ",");

    
    //Get Account Locked
    fprintf(fd, ",");
    
    //Get Account Expired
    dwError = Get_AccountExpired(pszFieldValue);
    if(dwError != -1)
    {
        fprintf(fd,"%d", dwError);
    }
	//Write Password Never Expires
    CsvGetField("pwdExpires", pLwtData, &pszFieldValue);
    CsvWriteField(fd, pszFieldValue);

	fprintf(fd, "\n");

	

   
    //Get Password Expired.
	return 0;


}

static
DWORD
CsvWriteField(
        FILE *fd,
        PSTR pszFieldValue)
{
    

    if (pszFieldValue != NULL)
    {
        //printf("\n %s \t %s", ppszCsvField[i], pszFieldValue);
        fprintf(fd, "%s", pszFieldValue);
        free (pszFieldValue);
		pszFieldValue = 0;
    }
            
    fprintf(fd, ",");
    return 0;
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
        fprintf(fd,"%s,", ppszCsvField[i]);
    }
    fprintf(fd,"\n");
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
            int *nMaxFieldCount,
            int *nMaxEntries,
            int tPosition 
            )
{

    char pszBuf[512];
    BOOL bFieldEnded = 0, bLineSeparator = 0, bLineFeed = 1;
    BOOL bFieldNameEnded = 0;
    DWORD dwError = LW_ERROR_SUCCESS;
    int nBufCount = 0;
    int tNumberOfField = 0;
    int tNumberOfEntries = 0, tSeekPosition = 0 ;
    int tLineEnded = 0, i = 0;
	int nTesting = 0;



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
                                tLineEnded = i - 2;
				tNumberOfField++;
	        		nTesting++;
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
		bLineSeparator = 0;
            }

        }
        /* If the read buffer ends right in end of the field*/
        if (nBufCount == 512)
        { 
            tSeekPosition += tLineEnded;
            bLineSeparator = 0;
            //bLineFeed = 0;
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

    if (pConfig->pszOutCsvName)
    {
        free(pConfig->pszOutCsvName);
        pConfig->pszOutCsvName = NULL;

    }
	return LW_ERROR_SUCCESS;
   
}




void
ShowUsage()
{
    printf("\n Usage : ldif2csv [options] \n");
    printf( "\n-s <servername> \n [ -o <outfile.csv> ]\n");        
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
    int tLength)
{
    PSTR pszFieldValue = NULL;
    DWORD dwError = LW_ERROR_SUCCESS;
    int tStrLength = 0;
    int i = 0;
    int j = 0;
    
    for (i =0; i < tLength; i++)
    {
        if(pszBuf[i] != '\r' && pszBuf[i] != '\n')
        {
            tStrLength++;
        }
    }

    
    pszFieldValue = (PSTR ) malloc(tStrLength+1);
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

    DWORD c;
    DWORD i, j;

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

    pOctetString = (PSTR )calloc(dwByteCount, 1);
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
CsvGetField(
        PSTR pszFieldName,
        PLWTDATA pData,
        PSTR *ppszFieldValue
        )
{

    PSTR pszFieldValue = NULL;
    PSTR pszFieldData = NULL;
    int tLen = 0;
    int i;
    DWORD dwError = LW_ERROR_SUCCESS;

	if(pData == NULL)
		return LW_ERROR_FIELD_NOT_FOUND;

	    
    for (i =0; i < pData->nFields; i++)
    {
        if (!strcmp(pszFieldName, "member"))
        {
            dwError = LwtDataGetGroupMembers(pData, &pszFieldValue);
            
            if (dwError == LW_ERROR_SUCCESS)
            {
                *ppszFieldValue = pszFieldValue;
				//printf("\n members :%s", pszFieldValue);
            
            }
            return dwError;
        }
        if(!strcmp(pszFieldName, pData->ppszFieldName[i]))
        {
            
            if (pData->ppszFieldValue[i] != NULL)
            {
                tLen = strlen(pData->ppszFieldValue[i]);
                pszFieldValue = (PSTR )malloc(tLen+1);
                memcpy(pszFieldValue, pData->ppszFieldValue[i], tLen);
                pszFieldValue[tLen] = '\0';
                *ppszFieldValue = pszFieldValue;
				
                return dwError;
            }
            else
            {
                *ppszFieldValue = 0;
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


	*ppszFieldValue = 0;
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
    int i =0, tLen = 0;
    int tKeyPosition = 0;
    
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
            pszFieldValue = (PSTR )malloc(tLen);
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
    int tLen = 0;
    int tBufferCopied = 0;
	
    for (i = 0; i < pData->nFields; i++)
    {
        if(!strcmp(pData->ppszFieldName[i], "member"))
        {
           tLen += GetLen(pData->ppszFieldValue[i]); 
        }
    }
    
    if (tLen > 0)    
    {
	   tLen = tLen + 2;
       pszFieldValue= (PSTR )malloc(tLen+1);
	   pszFieldValue[0] = '"';
	   tBufferCopied = 1;
    }
    else
    {
        return LW_ERROR_FIELD_NOT_FOUND; 
    }
    
    /* Concat and copy the field in to a single line buffer */
    for( i = 0; i < pData->nFields; i++)
    {
		size_t tLength = 0;
        if(!strcmp(pData->ppszFieldName[i], "member"))
        {
            PSTR pszField = pData->ppszFieldValue[i];            

            tLength = GetLen(pszField);
            if(tLength > 0)
            {
                memcpy(pszFieldValue + tBufferCopied, pszField, tLength);
            }
            tBufferCopied += tLength;
        }
		
    }
    if (tBufferCopied > 0)
    {
		pszFieldValue[tLen - 1] = '"';
		pszFieldValue[tLen] = '\0';
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

    int tIndex = 0;
    
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

DWORD ConvertBase64toSecurityDescriptor(PSTR pszEncodedString, PSTR *ppszSid)
{
	PSID pSid = NULL;
	char *pOctetString = NULL;
	DWORD dwByteCount = 0;
	//char *pszEncodedString = "AQUAAAAAAAUVAAAA/pDVbGXMHGcvxarLXQQAAA==";
	char *chSid = NULL;
	char *pszSid;
	DWORD dwError = 0;

	dwError = Base64Decode(pszEncodedString, &pOctetString, &dwByteCount);
	if(dwError != 0)
	{
		printf("\n Some problem");
		return 1;
	}
	pSid = malloc(dwByteCount);
	memcpy(pSid, pOctetString, dwByteCount);
	//pSid = (PSID)(pOctetString);
			 
	if(ConvertSidToStringSidA(pSid,&chSid))
	{
		int len = 0;
		printf("\n Converted SID is %s", chSid);
		len = strlen(chSid);
		pszSid = (char *)malloc(len + 1);
		memcpy(pszSid, chSid, len);
		pszSid[len] = '\0';
		*ppszSid = pszSid;
		free(pSid);
		pSid = NULL;
		return 0;
	}
	printf("\n Failed to convert");
	return 1;
}



static
BOOL
Get_AccountEnabled(char *pszAcountEnabled)
{
    DWORD dwUserAccoutControl = atol(pszAcountEnabled);

    if(dwUserAccoutControl != 0)
    {

        if (dwUserAccoutControl && ADS_UF_ACCOUNTDISABLE )
        {
            return 0;
        }
        else
        {
           return 1;
        }
    }
}


static
BOOL 
Get_CanChangePassword(char *pszAcountEnabled)
{
    DWORD dwUserAccoutControl = 0;
	dwUserAccoutControl = atol(pszAcountEnabled);

    if(dwUserAccoutControl != 0)
    {

        if (dwUserAccoutControl && ADS_PASSWD_CANT_CHANGE)
        {
            return 0;
        }
        else
        {
           return 1;
        }
    }
}

static
BOOL 
Get_PasswordNeverExpires(
						PSTR pszAcountEnabled
						)
{
    DWORD dwUserAccoutControl = atol(pszAcountEnabled);

    if(dwUserAccoutControl != 0)
    {

        if (dwUserAccoutControl && ADS_UF_DONT_EXPIRE_PASSWD )
        {
            return 1;
        }
        else
        {
           return 0;
        }
    }
	return 0;
}


static
BOOL
getNetBiosName(
			   PSTR pszDomainName
			   )
{
	int i = 0;
	for(i = 0; pszDomainName[i] != '\0'; i++)
	{
		if(pszDomainName[i] == '.')
		{
			strncpy(gpszNetBiosName, pszDomainName, i);
			gpszNetBiosName[i] = '\0';
			return 0;
		}
	}
	return 1;

}

static
BOOL 
Get_AccountExpired(
				PSTR pszAccountExpiresValue
					)
{
	return 0;
}
static
BOOL 
Get_PasswdLastSet(
					  PSTR pszField, 
					  PSTR *ppszValue
					  )
{
	return 0;
}
