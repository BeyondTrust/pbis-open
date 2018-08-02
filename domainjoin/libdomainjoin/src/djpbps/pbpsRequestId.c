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
 * The checkout and checkin reasons are displayed in Password Safe
 * web console.
 */
static PSTR pszDomainJoinCheckOutReason = "\"Reason\":\"PBIS Domainjoin\"";
static PSTR pszDomainJoinCheckInReason = "{\"Reason\":\"PBIS Domainjoin Done\"}";


/*
 *  Post Requests
 *
 */
DWORD PbpsApiRequestId(PbpsApi_t *pApi, DWORD *pdwRequestId)
{
   DWORD dwError = LW_ERROR_SUCCESS;
   DWORD dwRequestId = 0;
   CURLcode curlResult = CURLE_OK;
   long responseCode = 0;
   PSTR pszPostField = NULL;
   PSTR pszUrl = NULL;
   PSTR pszFunction = "Requests";
   PbpsApiManagedAccount_t *pAccount = NULL;
   responseBuffer_t responseBuffer;
   PSTR  pszEndPtr = NULL;

   responseBuffer.buffer = NULL;
   responseBuffer.size = 0;

   dwError = PbpsApiGetJoinAccount(pApi, pApi->config.pszJoinAccount, &pAccount);
   BAIL_ON_LW_ERROR(dwError);

   dwError = LwAllocateStringPrintf(
               &pszUrl, "%s/%s", pApi->config.pszUrlBase, pszFunction);
   BAIL_ON_LW_ERROR(dwError);

   dwError = LwAllocateStringPrintf(
               &pszPostField,
               "{\"AccessType\":\"View\",\"SystemID\":%d,\"AccountID\":%d,\"DurationMinutes\":%d,%s}",
                pAccount->dwSystemId,
                pAccount->dwAccountId,
                pApi->config.dwDurationMinutes,
                pszDomainJoinCheckOutReason);
   BAIL_ON_LW_ERROR(dwError);

   curlResult = curl_easy_setopt(pApi->session.pCurlHandle, CURLOPT_URL, pszUrl);
   BAIL_ON_CURL_ERROR(curlResult);

   dwError = PbpsApiCurlHeaderSet(pApi);
   BAIL_ON_LW_ERROR(dwError);

   curlResult = curl_easy_setopt(pApi->session.pCurlHandle, CURLOPT_POST, 0);
   BAIL_ON_CURL_ERROR(curlResult);

   curlResult = curl_easy_setopt(
                   pApi->session.pCurlHandle,
                   CURLOPT_POSTFIELDS,
                   (char *)pszPostField);
   BAIL_ON_CURL_ERROR(curlResult);

   curlResult = curl_easy_setopt(
                   pApi->session.pCurlHandle,
                   CURLOPT_POSTFIELDSIZE,
                   strlen(pszPostField));
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

   DJ_LOG_INFO("%s url:%s for %s \"SystemName\":\"%s\", \
                \"DomainName\":\"%s\",\"AccountName\":\"%s\"",
               pszFunction,
               pszUrl,
               pszPostField,
               pAccount->pszSystemName,
               pAccount->pszDomainName,
               pAccount->pszAccountName);

   curlResult = curl_easy_perform(pApi->session.pCurlHandle);
   BAIL_ON_CURL_ERROR(curlResult);

   curlResult = curl_easy_getinfo(
                   pApi->session.pCurlHandle,
                   CURLINFO_RESPONSE_CODE,
                   &responseCode);
  BAIL_ON_CURL_ERROR(curlResult);

  if ((responseCode != PBPS_HTTP_SUCCESS) &&
      (responseCode != PBPS_HTTP_SUCCESS_WITH_CONTENT))
  {
     DJ_LOG_ERROR("%s HTTP failed. Response code:%ld Response:%s(%d)",
                  pszFunction,
                  responseCode,
                  responseBuffer.buffer,
                  responseBuffer.size);
     dwError = PbpsHttpErrorToLwError(responseCode);
     BAIL_ON_LW_ERROR(dwError);
  }

  dwRequestId = (DWORD) LwStrtoll(responseBuffer.buffer, &pszEndPtr, 10);

  DJ_LOG_INFO("%s requestId:%d", pszFunction, dwRequestId);

  *pdwRequestId = dwRequestId;


cleanup:
   curl_easy_setopt(pApi->session.pCurlHandle, CURLOPT_WRITEFUNCTION, NULL);
   curl_easy_setopt( pApi->session.pCurlHandle, CURLOPT_WRITEDATA,NULL);

   LW_SAFE_FREE_STRING(pszUrl);

   if (responseBuffer.buffer)
     LwFreeMemory(responseBuffer.buffer);

   PbpsApiManagedAccountFree(pAccount);

   return dwError;

error:
  goto cleanup;

}

/*
 *  PUT  Request Checkin
 *
 */
DWORD PbpsApiRequestIdCheckin(PbpsApi_t *pApi, DWORD dwRequestId)
{
   DWORD dwError = LW_ERROR_SUCCESS;
   CURLcode curlResult = CURLE_OK;
   long responseCode = 0;
   PSTR pszUrl = NULL;
   responseBuffer_t responseBuffer;
   responseBuffer_t readBuffer;

   responseBuffer.buffer = NULL;
   responseBuffer.size = 0;

   readBuffer.buffer = pszDomainJoinCheckInReason;
   readBuffer.size = strlen(pszDomainJoinCheckInReason);

   dwError = LwAllocateStringPrintf(
               &pszUrl, "%s/Requests/%d/Checkin", pApi->config.pszUrlBase, dwRequestId);
   BAIL_ON_LW_ERROR(dwError);

   curlResult = curl_easy_setopt(pApi->session.pCurlHandle, CURLOPT_URL, pszUrl);
   BAIL_ON_CURL_ERROR(curlResult);

   dwError = PbpsApiCurlHeaderSet(pApi);
   BAIL_ON_LW_ERROR(dwError);

   curlResult = curl_easy_setopt(pApi->session.pCurlHandle, CURLOPT_UPLOAD, 1L);
   BAIL_ON_CURL_ERROR(curlResult);

   // Provide reason for PBPS checkin
   curlResult = curl_easy_setopt(pApi->session.pCurlHandle, CURLOPT_READFUNCTION, PbpsApiReadCallBackFunction);
   BAIL_ON_CURL_ERROR(curlResult);

   curlResult = curl_easy_setopt(pApi->session.pCurlHandle, CURLOPT_READDATA, (void*)&readBuffer);
   BAIL_ON_CURL_ERROR(curlResult);

   curlResult = curl_easy_setopt(pApi->session.pCurlHandle, CURLOPT_INFILESIZE,
                                 strlen(pszDomainJoinCheckInReason));
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

   DJ_LOG_INFO("Checkin url:%s", pszUrl);

   curlResult = curl_easy_perform(pApi->session.pCurlHandle);
   BAIL_ON_CURL_ERROR(curlResult);

   curlResult = curl_easy_getinfo(
                   pApi->session.pCurlHandle,
                   CURLINFO_RESPONSE_CODE,
                   &responseCode);
  BAIL_ON_CURL_ERROR(curlResult);

  if (responseCode != PBPS_HTTP_SUCCESS_WITHOUT_CONTENT)
  {
     DJ_LOG_ERROR("Checkin HTTP failed. Response code:%ld Response:%s(%d)",
                  responseCode,
                  responseBuffer.buffer,
                  responseBuffer.size);
     dwError = LW_ERROR_SUCCESS;
     goto error;
  }


cleanup:
   curl_easy_setopt(pApi->session.pCurlHandle, CURLOPT_UPLOAD, 0);
   curl_easy_setopt(pApi->session.pCurlHandle, CURLOPT_READFUNCTION, NULL);
   curl_easy_setopt(pApi->session.pCurlHandle, CURLOPT_READDATA, NULL);
   curl_easy_setopt(pApi->session.pCurlHandle, CURLOPT_INFILESIZE, 0);
   curl_easy_setopt(pApi->session.pCurlHandle, CURLOPT_WRITEFUNCTION, NULL);
   curl_easy_setopt(pApi->session.pCurlHandle, CURLOPT_WRITEDATA, NULL);

   LW_SAFE_FREE_STRING(pszUrl);

   if (responseBuffer.buffer)
     LwFreeMemory(responseBuffer.buffer);

   return dwError;

error:
  goto cleanup;

}

