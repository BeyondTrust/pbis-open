/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Beyondtrust Software    2018
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

#include "pbps-int.h"

/*
 * Using the config file, get credentials from the PasswordSafe server
 * for the user in the config file.
 * Updates pointer to username and password
 */
DWORD
PbpsApiGetCredentials(PSTR pszConfigFile,
                   PSTR *ppszUsername,
                   PSTR *ppszPassword)
{
   DWORD dwError = LW_ERROR_SUCCESS;
   PbpsApi_t *pApi = NULL;
   DWORD dwRequestId = 0;
   PSTR pszCredentials = NULL;
   BOOLEAN bDoCheckIn = FALSE;
   BOOLEAN bDoSignOut = FALSE;

   dwError =  PbpsApiInitialize(&pApi);
   BAIL_ON_LW_ERROR(dwError);

   dwError =  PbpsApiGetConfig(pApi, pszConfigFile);
   BAIL_ON_LW_ERROR(dwError);

   fprintf(stdout,
           "Requesting credentials for %s from %s\n",
           pApi->config.pszJoinAccount,
           pApi->config.pszUrlBase);

   dwError =  PbpsApiCurlDebugStart(pApi);
   BAIL_ON_LW_ERROR(dwError);

   dwError = PbpsApiSignIn(pApi);
   BAIL_ON_LW_ERROR(dwError);

   bDoSignOut = TRUE;

   dwError = PbpsApiManagedAccountsGet(pApi);
   BAIL_ON_LW_ERROR(dwError);

   dwError = PbpsApiRequestId(pApi, &dwRequestId);
   BAIL_ON_LW_ERROR(dwError);

   bDoCheckIn = TRUE;

   dwError = PbpsApiCredentialsGet(pApi, dwRequestId, &pszCredentials);
   BAIL_ON_LW_ERROR(dwError);

   dwError = LwStrDupOrNull(pApi->config.pszJoinAccount, ppszUsername);
   BAIL_ON_LW_ERROR(dwError);

   fprintf(stdout,
           "Successfully retrieved credentials for %s\n",
           pApi->config.pszJoinAccount);

   *ppszPassword  = pszCredentials;

cleanup:
   if (bDoCheckIn)
   {
      // Ignore the return code since we've already successfully retrieved 
      // pasword safe credentials. Error msgs would already have been 
      // logged.
      PbpsApiRequestIdCheckin(pApi, dwRequestId);
   }

   if (bDoSignOut)
      PbpsApiSignOut(pApi);

   PbpsApiCurlDebugStop(pApi);

   PbpsApiRelease(pApi);

   return dwError;

error:
   fprintf(stdout, "Failed to retrieved credentials\n");

   goto cleanup;
}


