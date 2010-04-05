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
 

 /*                                                               *
 *  retireves the information from the AD about the metrics       *
 *                                                                *  
 ******************************************************************/
#include "includes.h"


/*
 *
 * Print the Metric information. 
 */
static
VOID
Lwt_LsaPrintMetrics(
    PVOID pMetrics,
    DWORD dwLevel
    );



int 
get_metrics_main(
  int argc, 
  char *argv[]
  )
{
    HANDLE hLSAConnection = NULL;
    PLSA_METRIC_PACK_0 pMetrics_0 = NULL;
    PLSA_METRIC_PACK_1 pMetrics_1 = NULL;
    PTESTDATA pTestData = NULL;
    DWORD dwError = LW_ERROR_SUCCESS;


    dwError = Lwt_LsaTestSetup(argc,
                               argv,
                               &hLSAConnection,
                               &pTestData);
    BAIL(dwError);

    
    /*Start collect the groups for the level 1*/
    dwError = LsaGetMetrics(hLSAConnection, 
                            0,
                           (PVOID *) &pMetrics_0);
    BAIL(dwError);    
    if (pMetrics_0)
    {
        Lwt_LsaPrintMetrics((PVOID) pMetrics_0, 0);
    }
    
    
    dwError = LsaGetMetrics(hLSAConnection,
                            1,
                            (PVOID *)&pMetrics_1);
    BAIL(dwError);
    
    
    
    if (pMetrics_1)
    {
        Lwt_LsaPrintMetrics((PVOID) pMetrics_1, 1);
    } 
    
    
cleanup:
    if (pMetrics_0 != NULL)
    {
       LwFreeMemory(pMetrics_0); 
       pMetrics_0 = NULL;
    }

    if (pMetrics_1 != NULL)
    {
        LwFreeMemory(pMetrics_1);
        pMetrics_1 = NULL;
    }

    Lwt_LsaTestTeardown(&hLSAConnection,
                        &pTestData);
    return dwError;

error:
    goto cleanup;

}



static
VOID
Lwt_LsaPrintMetrics(
    PVOID pMetrics,
    DWORD dwLevel
 )
{
     if (dwLevel == 0)
     {
        PLSA_METRIC_PACK_0 pMetricPack = (PLSA_METRIC_PACK_0)pMetrics;
        fprintf(stdout, "Metric info (Level-0):\n");
        fprintf(stdout, "====================\n");
        fprintf(stdout, "Statistics:\n");
        fprintf(stdout, "====================\n");
        fprintf(stdout, "Failed Authenticatin count:  %llu\n", 
                                (unsigned long long)pMetricPack->failedAuthentications);
        fprintf(stdout, "Failed User Lookups by Name:  %llu\n", 
                                (unsigned long long)pMetricPack->failedUserLookupsByName);
        fprintf(stdout, "Failed User Lookups by ID:     %llu\n", 
                                (unsigned long long)pMetricPack->failedAuthentications);
        fprintf(stdout, "Failed Group Lookups by Name:  %llu\n", 
                                (unsigned long long)pMetricPack->failedGroupLookupsByName);
        fprintf(stdout, "Failed Group Lookups by ID:  %llu\n", 
                                (unsigned long long)pMetricPack->failedGroupLookupsById);
        fprintf(stdout, "Failed Open Session  :       %llu\n", 
                                (unsigned long long)pMetricPack->failedOpenSession);
        fprintf(stdout, "Failed Close Session :       %llu\n", 
                                (unsigned long long)pMetricPack->failedCloseSession);
        fprintf(stdout, "Failed Change Passwords:      %llu\n", 
                                (unsigned long long)pMetricPack->failedChangePassword);
    }
    if(dwLevel == 1)
    {
        PLSA_METRIC_PACK_1 pMetricPack = (PLSA_METRIC_PACK_1)pMetrics;
        fprintf(stdout, "\n\n\nMetric info (Level-1):\n");
        fprintf(stdout, "====================\n");
        fprintf(stdout, "Statistics:\n");
        fprintf(stdout, "====================\n");
        fprintf(stdout, "Successful Authenticatin count:  %llu\n", 
                                (unsigned long long)pMetricPack->successfulAuthentications);
        fprintf(stdout, "Failed Authenticatin count:      %llu\n", 
                                (unsigned long long)pMetricPack->failedAuthentications);
        fprintf(stdout, "root Authentication Count:       %llu\n", 
                                (unsigned long long)pMetricPack->rootUserAuthentications);
        fprintf(stdout, "Successful User lookups by Name: %llu\n", 
                                (unsigned long long)pMetricPack->successfulUserLookupsByName);
        fprintf(stdout, "Failed User Lookups by Name:     %llu\n", 
                                (unsigned long long)pMetricPack->failedUserLookupsByName);
        fprintf(stdout, "Successful User Lookups by ID:   %llu\n", 
                                (unsigned long long)pMetricPack->successfulUserLookupsById);
        fprintf(stdout, "Failed User Lookups by ID:       %llu\n", 
                                (unsigned long long)pMetricPack->failedAuthentications);
        fprintf(stdout, "Successful Group Lookup by Name: %llu\n", 
                                (unsigned long long)pMetricPack->successfulGroupLookupsByName);
        fprintf(stdout, "Failed Group Lookups by Name:    %llu\n", 
                                (unsigned long long)pMetricPack->failedGroupLookupsByName);
        fprintf(stdout, "Successful Group Lookups by ID:  %llu\n", 
                                (unsigned long long)pMetricPack->successfulGroupLookupsById);
        fprintf(stdout, "Failed Group Lookups by ID:      %llu\n", 
                                (unsigned long long)pMetricPack->failedGroupLookupsById);
        fprintf(stdout, "Successful Open Session:         %llu\n", 
                                (unsigned long long)pMetricPack->successfulOpenSession);
        fprintf(stdout, "Failed Open Session  :           %llu\n", 
                                (unsigned long long)pMetricPack->failedOpenSession);
        fprintf(stdout, "Successful Close Session :       %llu\n", 
                                (unsigned long long)pMetricPack->successfulCloseSession);
        fprintf(stdout, "Failed Close Session :           %llu\n", 
                                (unsigned long long)pMetricPack->failedCloseSession);
        fprintf(stdout, "Successful Change Passwords:     %llu\n", 
                                (unsigned long long)pMetricPack->successfulChangePassword);
        fprintf(stdout, "Failed Change Passwords:         %llu\n", 
                                (unsigned long long)pMetricPack->failedChangePassword);

    }


}
