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
 * for the domain join user in the config file.
 * Updates pointer to username, password and a API handle subsequently used 
 * to release the credentials with PasswordSafe.
 */

DWORD
PbpsApiCredentialGet(PSTR pszConfigFile,
                     PSTR *ppszUsername,
                     PSTR *ppszPassword,
                     PPbpsApiHandle_t *ppPbpsApiHandle)
{

   DWORD dwError = LW_ERROR_SUCCESS;
   PbpsApi_t *pApi = NULL;
   DWORD dwRequestId = 0;
   PSTR pszCredentials = NULL;

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

   dwError = PbpsApiManagedAccountsGet(pApi);
   BAIL_ON_LW_ERROR(dwError);

   dwError = PbpsApiRequestId(pApi, &dwRequestId);
   BAIL_ON_LW_ERROR(dwError);

   dwError = PbpsApiCredentialsGet(pApi, dwRequestId, &pszCredentials);
   BAIL_ON_LW_ERROR(dwError);

   dwError = LwStrDupOrNull(pApi->config.pszJoinAccount, ppszUsername);
   BAIL_ON_LW_ERROR(dwError);

   *ppszPassword = pszCredentials;
   *ppPbpsApiHandle = pApi;

   fprintf(stdout,
           "Successfully retrieved credentials for %s\n",
           pApi->config.pszJoinAccount);

cleanup:

   return dwError;

error:

    fprintf(stderr, "Failed to retrieve credentials\n");

    LW_SAFE_FREE_STRING(pszCredentials);
    PbpsApiRelease(pApi);

    goto cleanup;
}



VOID
PbpsApiCredentialRelease(PPbpsApiHandle_t *ppPbpsApiHandle)
{
   PbpsApi_t *pApi = NULL;

   if (*ppPbpsApiHandle == NULL)
     goto cleanup;

   pApi = *ppPbpsApiHandle;

   // Ignore the return code since we've already successfully retrieved 
   // pasword safe credentials. Error msgs would already have been 
   // logged.
   PbpsApiRequestIdCheckin(pApi);

   PbpsApiSignOut(pApi);

   PbpsApiCurlDebugStop(pApi);

   PbpsApiRelease(pApi);

   *ppPbpsApiHandle = NULL;

cleanup:

   return;
}

