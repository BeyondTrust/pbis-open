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
 * GET Credentials/{requestId}
 *    dwRequestId: request id from POST Requests
 *    ppCredentials: Returns the credentials which must be freed by the
 *                   caller.
 *  
 */
DWORD
PbpsApiCredentialsGet(
   PbpsApi_t *pApi,
   DWORD dwRequestId,
   PSTR *ppszCredentials)
{
   DWORD dwError = LW_ERROR_SUCCESS;
   CURLcode curlResult = CURLE_OK;
   long responseCode = 0;
   PSTR pszUrl = NULL;
   PSTR pszFunction = "Credentials";
   PSTR pszCredentials = NULL;
   responseBuffer_t responseBuffer;

   responseBuffer.buffer = NULL;
   responseBuffer.size = 0;

   dwError = LwAllocateStringPrintf(
               &pszUrl, "%s/%s/%d", pApi->config.pszUrlBase, pszFunction, dwRequestId);
   BAIL_ON_LW_ERROR(dwError);

   curlResult = curl_easy_setopt(pApi->session.pCurlHandle, CURLOPT_URL, pszUrl);
   BAIL_ON_CURL_ERROR(curlResult);

   curlResult = curl_easy_setopt(pApi->session.pCurlHandle, CURLOPT_HTTPGET, 1);
   BAIL_ON_CURL_ERROR(curlResult);

   curlResult = curl_easy_setopt(
                   pApi->session.pCurlHandle,
                   CURLOPT_WRITEFUNCTION,
                   PbpsApiWriteCallBackFunction);
   BAIL_ON_CURL_ERROR(curlResult);

   curlResult = curl_easy_setopt(
                   pApi->session.pCurlHandle,
                   CURLOPT_WRITEDATA,
                   (void*) &responseBuffer);
   BAIL_ON_CURL_ERROR(curlResult);

   DJ_LOG_INFO("%s get url:%s", pszFunction, pszUrl);

   curlResult = curl_easy_perform(pApi->session.pCurlHandle);
   BAIL_ON_CURL_ERROR(curlResult);

   curlResult = curl_easy_getinfo(
                   pApi->session.pCurlHandle,
                   CURLINFO_RESPONSE_CODE,
                   &responseCode);
  BAIL_ON_CURL_ERROR(curlResult);

  if (responseCode != PBPS_HTTP_SUCCESS)
  {
     DJ_LOG_ERROR("HTTP failed. Response code:%ld Response:%s(%d)",
                  responseCode,
                  responseBuffer.buffer,
                  responseBuffer.size);
     dwError = PbpsHttpErrorToLwError(responseCode);
     BAIL_ON_LW_ERROR(dwError);
  }

  if ((responseBuffer.size > 0) && (responseBuffer.buffer))
  {
     dwError = LwAllocateStringPrintf(
                 &pszCredentials, "%s", responseBuffer.buffer);
     BAIL_ON_LW_ERROR(dwError);

     LwStripTrailingCharacters(pszCredentials, '"');
     LwStripLeadingCharacters(pszCredentials, '"');
  }

cleanup:

  *ppszCredentials = pszCredentials;

  if (responseBuffer.buffer)
    LwFreeMemory(responseBuffer.buffer);

   LW_SAFE_FREE_STRING(pszUrl);

   return dwError;

error:
   goto cleanup;
}

