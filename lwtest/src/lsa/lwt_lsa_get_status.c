/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 * lwt_lsa_get_status <Config File>
 *
 * Verify that information returned by AD matches the CSV file.
 *
 *
 */


#include "includes.h"




#define LSA_UNPROV_MODE "Un-Provisioned"
#define LSA_UNKNOWN_MODE "Unknown"
#define LSA_DEFAULT_MODE "Default Cell"
#define LSA_DEFAULT_NONCELL_MODE "Named Cell"
#define LSA_LOCAL_SYSTEM_MODE "Local System"



#define LSA_SUB_MODE_SCHEMA "Schema"
#define LSA_SUB_MODE_NON_SCHEMA "Non-Schema Mode"
#define LSA_SUB_MODE_UNKNOWN "Unknown Mode"

#define LSA_DC_STATUS_UNKNOWN "Unknown"
#define LSA_DC_STATUS_ONLINE "Online"
#define LSA_DC_STATUS_OFFLINE "Offline"
#define LSA_DC_STATUS_FORCED_OFFLINE "Forced Offline"
    


/*
 *
 * Get the LsaStatus Information 
 *
 */
DWORD
Lwt_LsaGetStatus(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    );


/*
 *
 * Verify information returned by
 * LsaGetStatus is same as the
 * information in CSV
 *
 */
#if 0
static
DWORD
VerifyInformation(
    PTESTDATA pTestData,
    PLSASTATUS pLsaStatus
    );
#endif


/*
 *
 * Print DC information
 *
 */              
static
DWORD
PrintDCInformation(
        PLSASTATUS pLsaStatus
        );



static
PSTR
GetStatus(
        LsaAuthProviderStatus status
        );

static
PSTR
GetMode(
    LsaAuthProviderMode mode
    );

static
PSTR
GetSubMode(
    LsaAuthProviderSubMode subMode
    );
    


    


int 
get_status_main(
    int argc, 
    char *argv[]
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    HANDLE hLsaConnection = NULL;
    PTESTDATA pTestData = NULL;
    
    dwError = Lwt_LsaTestSetup(argc, argv, &hLsaConnection, &pTestData);
    if ( dwError )
        goto error;

    dwError = Lwt_LsaGetStatus(hLsaConnection, pTestData);
    if(dwError)
        goto error;

cleanup:

    Lwt_LsaTestTeardown(&hLsaConnection, &pTestData);
    
    return LwtMapErrorToProgramStatus(dwError);

error:
    goto cleanup;

}

/*
 *
 * Get the LsaStatus Information 
 *
 */

DWORD
Lwt_LsaGetStatus(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;

    PLSASTATUS pLsaStatus = NULL;
 
    if ( !pTestData)
    {
        dwError = LW_ERROR_TEST_SKIPPED;
        goto error;
    }

    dwLocalError = LsaGetStatus(hLsaConnection, &pLsaStatus);
    BAIL_ON_TEST_BROKE(dwLocalError);

    dwLocalError = PrintDCInformation(pLsaStatus);
    if ( dwLocalError )
    {
        dwError = LW_ERROR_TEST_FAILED;
        goto error;
    }

cleanup:

    if (pLsaStatus)
    {
        LsaFreeStatus(pLsaStatus);
    }
    
    return dwError;
error:
    goto cleanup;
}
 



static
DWORD
PrintDCInformation(
        PLSASTATUS pLsaStatus
       )
{

    
    PLSA_AUTH_PROVIDER_STATUS pAuthProvider = pLsaStatus->pAuthProviderStatusList;

    fprintf(stderr,"\nTesting the likewise test suit for the  domain %s\n", pAuthProvider->pszDomain);
    fprintf(stderr, "Properties\n");
    fprintf(stderr, "\t\tOperating Mode : %s\n", GetMode(pAuthProvider->mode));
    fprintf(stderr, "\t\tOperating Submode : %s\n", GetSubMode(pAuthProvider->subMode));
    fprintf(stderr, "\t\tCurrent Status : %s\n", GetStatus(pAuthProvider->status));
    if (pAuthProvider->pszForest)
    {
        fprintf(stderr, "\t\tForest Name: %s\n", pAuthProvider->pszForest);
    }
    if (pAuthProvider->pszCell)
    {
        fprintf(stderr, "\t\tCell Name : %s\n",   pAuthProvider->pszCell);
    }
       
    return LW_ERROR_SUCCESS;


}


static
PSTR
GetMode(
    LsaAuthProviderMode mode   
    )
{
    PSTR pszMode = NULL;
    switch(mode)
    {
        case LSA_PROVIDER_MODE_UNKNOWN :
                          pszMode = LSA_UNKNOWN_MODE;
                          break;

        case LSA_PROVIDER_MODE_UNPROVISIONED : 
                          pszMode = LSA_UNPROV_MODE;
                          break;

       case LSA_PROVIDER_MODE_DEFAULT_CELL :
                         pszMode = LSA_DEFAULT_MODE;
                         break;

        case LSA_PROVIDER_MODE_NON_DEFAULT_CELL :
                         pszMode = LSA_DEFAULT_NONCELL_MODE;
                         break;
        
        case LSA_PROVIDER_MODE_LOCAL_SYSTEM :
                        pszMode = LSA_LOCAL_SYSTEM_MODE;
                        break;
    }
    return pszMode;
}


static
PSTR
GetSubMode(
    LsaAuthProviderSubMode mode   
    )
{
    PSTR pszMode = NULL;
    switch(mode)
    {
        case LSA_AUTH_PROVIDER_SUBMODE_UNKNOWN :
                          pszMode = LSA_SUB_MODE_UNKNOWN;
                          break;

        case LSA_AUTH_PROVIDER_SUBMODE_SCHEMA :
                          pszMode = LSA_SUB_MODE_SCHEMA;
                          break;
        
        case LSA_AUTH_PROVIDER_SUBMODE_NONSCHEMA :
                         pszMode = LSA_SUB_MODE_NON_SCHEMA;
                         break;
    }
    return pszMode;
}
        

    
                        
static
PSTR
GetStatus(
    LsaAuthProviderStatus status   
    )
{
    PSTR pszStatus = NULL;
    switch(status)
    {
        case LSA_AUTH_PROVIDER_STATUS_UNKNOWN :
                          pszStatus = LSA_DC_STATUS_UNKNOWN;
                          break;

        case LSA_AUTH_PROVIDER_STATUS_ONLINE :
                          pszStatus = LSA_DC_STATUS_ONLINE;
                          break;
        
        case LSA_AUTH_PROVIDER_STATUS_OFFLINE :
                         pszStatus = LSA_DC_STATUS_OFFLINE;
                         break;

        case LSA_AUTH_PROVIDER_STATUS_FORCED_OFFLINE :
                        pszStatus = LSA_DC_STATUS_FORCED_OFFLINE;
                        break;
    }
    return pszStatus;
}
                    



#if 0
static
DWORD
VerifyInformation(
    PDOMAIN pDomain,
    PLSASTATUS pLsaStatus
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    PLSASTATUS pLsaStatus = NULL;
    PLSA_AUTH_PROVIDER_STATUS pLsaAuthProviderStatus = NULL;
    
    PCSTR pszTestDescription = 
        "Check LSASTATUS from LsaGetStatus has the expected information";
    PCSTR pszTestAPIs = 
        "LsaGetStatus,"
        "LsaFreeStatus";
    
    char szTestMsg[128] = { 0 };
    size_t iCount = 0;
    

    if (pLsaStatus->dwCount)
    {
        for (iCount = 0; iCount < pLsaStatus->dwCount; iCount++)
        {
            pLsaAuthProviderStatus =
                &pLsaStatus->pAuthProviderStatusList[iCount];

            if ( !IsNullOrEmpty(pAuthProviderStatus->pszId) && 
                strcmp(pAuthProviderStatus->pszId, pDomain->pszId) )
            {
                dwError = LW_ERROR_TEST_FAILED;
                snprintf(szTestMsg, sizeof(szTestMsg), 
                    "got inconsistant info for \
                    %s for the field pszId\n", pDomain->pszId);
                goto error;
            }

            if ( !IsNullOrEmpty(pAuthProviderStatus->pszDomain) && 
                strcmp(pAuthProviderStatus->pszDomain, pDomain->pszNetBiosName) )
            {
                dwError = LW_ERROR_TEST_FAILED;
                snprintf(szTestMsg, sizeof(szTestMsg), 
                    "got inconsistant info for \
                    %s for the field pszDomain\n", pDomain->pszNetBiosName);
                goto error;
            }

            if ( !IsNullOrEmpty(pAuthProviderStatus->pszForest) && 
                strcmp(pAuthProviderStatus->pszForest, pDomain->pszForest) )
            {
                dwError = LW_ERROR_TEST_FAILED;
                snprintf(szTestMsg, sizeof(szTestMsg), 
                    "got inconsistant info for \
                    %s for the field pszForest\n", pDomain->pszForest);
                goto error;
            }

            if ( !IsNullOrEmpty(pAuthProviderStatus->pszSite) && 
                strcmp(pAuthProviderStatus->pszSite, pDomain->pszSite) )
            {
                dwError = LW_ERROR_TEST_FAILED;
                snprintf(szTestMsg, sizeof(szTestMsg), 
                    "got inconsistant info for \
                    %s for the field pszSite\n", pDomain->pszSite);
                goto error;
            }

            if ( !IsNullOrEmpty(pAuthProviderStatus->pszCell) && 
                strcmp(pAuthProviderStatus->pszCell, pDomain->pszCell) )
            {
                dwError = LW_ERROR_TEST_FAILED;
                snprintf(szTestMsg, sizeof(szTestMsg), 
                    "got inconsistant info for \
                    %s for the field pszSite\n", pDomain->pszCell);
                goto error;
            }

            if ( pAuthProviderStatus->dwNumTrustedDomains )
            {
                size_t iDomain = 0;
                
                for ( iDomain = 0; iDomain < pAuthProviderStatus->dwNumTrustedDomains; iDomain++ )
                {
                    dwLocalError = VerifyTrustedDomainInfo(pTestData, pAuthProviderStatus->pTrustedDomainInfoArray[iDomain]);
                    if ( dwLocalError )
                    {
                        dwError = LW_ERROR_TEST_FAILED;
                        snprintf(szTestMsg, sizeof(szTestMsg), 
                            "got inconsistant info \
                            for the field pTrustedDomainInfoArray");
                        goto error;
                    }
                }
            }
        }
    }


cleanup:

    LWT_LOG_TEST(szTestMsg);
    
    return dwError;
error:
    goto cleanup;
}

/*
 *
 * Verify information in 
 * LW_LSA_TRUSTED_DOMAIN_INFO
 * against CSV file
 *
 */

static
DWORD
VerifyTrustedDomainInfo(
    PDOMAIN pDomain,
    PLW_LSA_TRUSTED_DOMAIN_INFO pTrustedDomainInfo
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
 
    PCSTR pszTestDescription = 
        "Check LSASTATUS from LsaGetStatus has the expected information";
    PCSTR pszTestAPIs = 
        "LsaGetStatus,"
        "LsaFreeStatus";
    
    char szTestMsg[128] = { 0 };
 
    if ( !IsNullOrEmpty(pTrustedDomainInfo->pszDnsdomain) && 
        strcmp(pTrustedDomainInfo->pszDnsDomain, pDomain->pszFQDN) )
    {
        dwError = LW_ERROR_TEST_FAILED;
            snprintf(szTestMsg, sizeof(szTestMsg), 
            "got inconsistant info for \
            %s for the field pszDnsDomain\n", pDomain->pszFQDN);
        goto error;
    }

    if ( !IsNullOrEmpty(pTrustedDomainInfo->pszNetbiosDomain) && 
        strcmp(pTrustedDomainInfo->pszNetbiosDomain, pDomain->pszNetBiosName) )
    {
        dwError = LW_ERROR_TEST_FAILED;
            snprintf(szTestMsg, sizeof(szTestMsg), 
            "got inconsistant info for \
            %s for the field pszNetBiosName\n", pDomain->pszNetBiosName);
        goto error;
    }

    if ( !IsNullOrEmpty(pTrustedDomainInfo->pszTrusteeDnsDomain) && 
        strcmp(pTrustedDomainInfo->pszTrusteeDnsDomain, pDomain->pszTrusteeDnsDomain) )
    {
        dwError = LW_ERROR_TEST_FAILED;
            snprintf(szTestMsg, sizeof(szTestMsg), 
            "got inconsistant info for \
            %s for the field pszTrusteeDnsDomain\n", pDomain->pszTrusteeDnsDomain);
        goto error;
    }


    if ( !IsNullOrEmpty(pTrustedDomainInfo->pszDomainSID) && 
        strcmp(pTrustedDomainInfo->pszDomainSID, pDomain->pszDomainSID) )
    {
        dwError = LW_ERROR_TEST_FAILED;
            snprintf(szTestMsg, sizeof(szTestMsg), 
            "got inconsistant info for \
            %s for the field pszDomainSID\n", pDomain->pszDomainSID);
        goto error;
    }

    if ( !IsNullOrEmpty(pTrustedDomainInfo->pszDomainGUID) && 
        strcmp(pTrustedDomainInfo->pszDomainGUID, pDomain->pszDomainGUID) )
    {
        dwError = LW_ERROR_TEST_FAILED;
            snprintf(szTestMsg, sizeof(szTestMsg), 
            "got inconsistant info for \
            %s for the field pszDomainGUID\n", pDomain->pszDomainGUID);
        goto error;
    }

    if ( !IsNullOrEmpty(pTrustedDomainInfo->pszForestName) && 
        strcmp(pTrustedDomainInfo->pszForestName, pDomain->pszForestName) )
    {
        dwError = LW_ERROR_TEST_FAILED;
            snprintf(szTestMsg, sizeof(szTestMsg), 
            "got inconsistant info for \
            %s for the field pszForestName\n", pDomain->pszForestName);
        goto error;
    }

    if ( !IsNullOrEmpty(pTrustedDomainInfo->pszClientSiteName) && 
        strcmp(pTrustedDomainInfo->pszClientSiteName, pDomain->pszClientSiteName) )
    {
        dwError = LW_ERROR_TEST_FAILED;
            snprintf(szTestMsg, sizeof(szTestMsg), 
            "got inconsistant info for \
            %s for the field pszForestName\n", pDomain->pszClientSiteName);
        goto error;
    }

    if ( pTrustedDomainInfo->dwTrustFlags != pDomain->dwTrustFlags ) 
    {
        dwError = LW_ERROR_TEST_FAILED;
            snprintf(szTestMsg, sizeof(szTestMsg), 
            "got inconsistant info for \
            %lu for the field dwTrustFlags\n",
            (unsigned long)pDomain->dwTrustFlags);
        goto error;
    }
    
    if ( pTrustedDomainInfo->dwTrustType != pDomain->dwTrustType ) 
    {
        dwError = LW_ERROR_TEST_FAILED;
            snprintf(szTestMsg, sizeof(szTestMsg), 
            "got inconsistant info for \
            %lu for the field dwTrustType\n",
            (unsigned long)pDomain->dwTrusType);
        goto error;
    }

    if ( pTrustedDomainInfo->dwTrustAttributes != pDomain->dwTrustAttributes ) 
    {
        dwError = LW_ERROR_TEST_FAILED;
            snprintf(szTestMsg, sizeof(szTestMsg), 
            "got inconsistant info for \
            %lu for the field dwTrustAttributes\n",
            (unsigned long)pDomain->dwTrustAttributes);
        goto error;
    }

    if ( pTrustedDomainInfo->dwTrustDirection != pDomain->dwTrustDirection ) 
    {
        dwError = LW_ERROR_TEST_FAILED;
            snprintf(szTestMsg, sizeof(szTestMsg), 
            "got inconsistant info for \
            %lu for the field dwTrustDirection\n",
            (unsigned long)pDomain->dwTrustDirection);
        goto error;
    }

    if ( pTrustedDomainInfo->dwTrustMode != pDomain->dwTrustMode ) 
    {
        dwError = LW_ERROR_TEST_FAILED;
            snprintf(szTestMsg, sizeof(szTestMsg), 
            "got inconsistant info for \
            %lu for the field dwTrustMode\n",
            (unsigned long)pDomain->dwTrustMode);
        goto error;
    }

    if ( pTrustedDomainInfo->dwDomainFlags != pDomain->dwDomainFlags ) 
    {
        dwError = LW_ERROR_TEST_FAILED;
            snprintf(szTestMsg, sizeof(szTestMsg), 
            "got inconsistant info for \
            %lu for the field dwDomainFlags\n",
            (unsigned long)pDomain->dwDomainFlags);
        goto error;
    }
    
    if( pTrustedDomainInfo->pDCInfo )
    {
        PLSA_DC_INFO pDCInfo = pTrustedDomainInfo->pDCInfo;
        
        if ( !IsNullOrEmpty(pDCInfo->pszName) && 
            strcmp(pDCInfo->pszName, pDomain->pDCInfo->pszName) )
        {
            dwError = LW_ERROR_TEST_FAILED;
                snprintf(szTestMsg, sizeof(szTestMsg), 
                "got inconsistant info for \
                %s for the field pDCInfo->pszName\n", pDomain->pDCInfo->pszName);
            goto error;
        }

        if ( !IsNullOrEmpty(pDCInfo->pszAddress) && 
            strcmp(pDCInfo->pszAddress, pDomain->pDCInfo->pszAddress) )
        {
            dwError = LW_ERROR_TEST_FAILED;
                snprintf(szTestMsg, sizeof(szTestMsg), 
                "got inconsistant info for \
                %s for the field pDCInfo->pszAddress\n", pDomain->pDCInfo->pszAddress);
            goto error;
        }

        if ( !IsNullOrEmpty(pDCInfo->pszSiteName) && 
            strcmp(pDCInfo->pszSiteName, pDomain->pDCInfo->pszSiteName) )
        {
            dwError = LW_ERROR_TEST_FAILED;
                snprintf(szTestMsg, sizeof(szTestMsg), 
                "got inconsistant info for \
                %s for the field pDCInfo->pszSiteName\n", pDomain->pDCInfo->pszSiteName);
            goto error;
        }

        if( pDCInfo->dwFlags != pDomain->pDCInfo->dwFlags )
        {
            dwError = LW_ERROR_TEST_FAILED;
                snprintf(szTestMsg, sizeof(szTestMsg), 
                "got inconsistant info for \
                %lu for the field pDCInfo->dwFlags\n",
                (unsigned long)pDomain->pDCInfo->dwFlags);
            goto error;
        }

    }

    if ( pTrustedDomainInfo->pGCInfo )
    {
        /* do the same test done for DCInfo.  write common code block */ 
    }
 
cleanup:

    LWT_LOG_TEST(szTestMsg);
    
    return dwError;
error:
    goto cleanup;
}

#endif
