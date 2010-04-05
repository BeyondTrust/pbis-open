#include "includes.h"

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
    pData = NULL;
}

DWORD
Lwt_LsaTestGetValue(
    PLWTDATA pLwData,
    PCSTR pszFieldName,
    PSTR *ppszFieldValue)
{
    size_t tIndex = 0;
    PSTR pszFieldValue = NULL;

    for (tIndex = 0; tIndex < pLwData->nFields; tIndex++)
    {
        if (!strcmp(pLwData->ppszFieldName[tIndex], pszFieldName))
        {
            
            if (pLwData->ppszFieldValue[tIndex] != NULL)
            {
                DWORD dwLen = strlen(pLwData->ppszFieldValue[tIndex]);
                pszFieldValue = malloc(dwLen+1);
                if (pszFieldValue == NULL)
                {
                    return LW_ERROR_OUT_OF_MEMORY;
                }
                memcpy(pszFieldValue, pLwData->ppszFieldValue[tIndex], dwLen);
                pszFieldValue[dwLen] ='\0';
                *ppszFieldValue = pszFieldValue;
                return LW_ERROR_SUCCESS;
            }
            else
            {
                break;
            }
        }
    }
    return LW_ERROR_CSV_NO_SUCH_FIELD;
}

