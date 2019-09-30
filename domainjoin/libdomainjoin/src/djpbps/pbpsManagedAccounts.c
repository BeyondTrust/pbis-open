/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

#include "pbps-int.h"

/*
 * This module has functions to retrieved from Password Safe in json
 * format a list of managed accounts. This list is parsed and translated
 * to a linked list of managed accounts.
 *
 *
 * GET ManagedAccounts
 *
 */


VOID PbpsApiManagedAccountFree(PbpsApiManagedAccount_t *pAccount)
{

   if (!pAccount)
     goto cleanup;

   LW_SAFE_FREE_STRING(pAccount->pszSystemName);
   LW_SAFE_FREE_STRING(pAccount->pszDomainName);
   LW_SAFE_FREE_STRING(pAccount->pszAccountName);
   LW_SAFE_FREE_STRING(pAccount->pszDomainAccountName);
   LwFreeMemory(pAccount);

cleanup:
   return;
}

static
VOID PbpsApiManagedAccountItemFree(PVOID pItem,
                               PVOID pUserData)
{
   PbpsApiManagedAccount_t *pAccount = (PbpsApiManagedAccount_t *) pItem;

   PbpsApiManagedAccountFree(pAccount);

   return;
}

VOID PbpsManagedAccountListFree(PLW_DLINKED_LIST pManagedAccountList)
{
   LwDLinkedListForEach(pManagedAccountList,
                        &PbpsApiManagedAccountItemFree,
                        NULL);

   LwDLinkedListFree(pManagedAccountList);
   return;
}

static
VOID
PbpsApiManagedAccountPrint(PVOID pItem,
                           PVOID pUserData)
{
   PbpsApiManagedAccount_t *pAccount = (PbpsApiManagedAccount_t *) pItem;

   DJ_LOG_VERBOSE("AccountName:%s\n\tSystemName:%s DomainName:%s\n\tPlatformId:%d SystemId:%d AccountId:%d\n",
                 pAccount->pszAccountName,
                 pAccount->pszSystemName,
                 pAccount->pszDomainName ? pAccount->pszDomainName : "null",
                 pAccount->dwPlatformId,
                 pAccount->dwSystemId,
                 pAccount->dwAccountId);

   return;
}

/*
 *  Log a linked list of managed accounts.
 */
static
VOID
PbpsApiManagedAccountsPrint(PLW_DLINKED_LIST pManagedAccountList)
{
   LwDLinkedListForEach(pManagedAccountList,
                        &PbpsApiManagedAccountPrint,
                        NULL);
   return;
}


/*
 * GET ManagedAccounts
 */
DWORD
PbpsApiManagedAccountsGet(PbpsApi_t *pApi)
{
   DWORD dwError = LW_ERROR_SUCCESS;
   CURLcode curlResult = CURLE_OK;
   long responseCode = 0;
   PSTR pszUrl = NULL;
   PSTR pszFunction = "ManagedAccounts";
   responseBuffer_t responseBuffer;
   PLW_DLINKED_LIST pTmpManagedAccounts = NULL;

   responseBuffer.buffer = NULL;
   responseBuffer.size = 0;

   dwError = LwAllocateStringPrintf(
               &pszUrl, "%s/%s", pApi->config.pszUrlBase, pszFunction);
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

   DJ_LOG_INFO("url:%s", pszUrl);

   curlResult = curl_easy_perform(pApi->session.pCurlHandle);
   BAIL_ON_CURL_ERROR(curlResult);

   curlResult = curl_easy_getinfo(
                   pApi->session.pCurlHandle,
                   CURLINFO_RESPONSE_CODE,
                   &responseCode);
  BAIL_ON_CURL_ERROR(curlResult);

  if (responseCode != PBPS_HTTP_SUCCESS)
  {
     DJ_LOG_ERROR("%s HTTP failed. Response code:%ld Response:%s(%d)",
                  pszFunction,
                  responseCode,
                  responseBuffer.buffer,
                  responseBuffer.size);
     dwError = PbpsHttpErrorToLwError(responseCode);
     BAIL_ON_LW_ERROR(dwError);
  }

  DJ_LOG_VERBOSE("\tManaged Accounts: size:%d\n \t%s\n",
                 responseBuffer.size,
                 responseBuffer.buffer);

  if ((responseBuffer.size > 0) && (responseBuffer.buffer))
  {
     dwError = PbpsApiManagedAccountsParse(responseBuffer.buffer, &pTmpManagedAccounts);
     BAIL_ON_LW_ERROR(dwError);

     if (pApi->session.pManagedAccountList)
        PbpsManagedAccountListFree(pApi->session.pManagedAccountList);

     pApi->session.pManagedAccountList = pTmpManagedAccounts;

     PbpsApiManagedAccountsPrint(pTmpManagedAccounts);
  }

cleanup:

   curl_easy_setopt(pApi->session.pCurlHandle, CURLOPT_WRITEFUNCTION, NULL);
   curl_easy_setopt(pApi->session.pCurlHandle, CURLOPT_WRITEDATA, NULL);

  if (responseBuffer.buffer)
    LwFreeMemory(responseBuffer.buffer);

   LW_SAFE_FREE_STRING(pszUrl);

   return dwError;

error:
   goto cleanup;
}


/*
 *  GET <base>/ManagedAccounts returns a list of managed accounts in
 *  json format. This function parses the input and returns a linked
 *  list of managed accounts.
 *
 * Convert a string of multiple managed accounts into a linked list.
 * Input: pszManagedAccount. Sample input:
 *
 * {"PlatformID":4,"SystemId":5,"SystemName":"build-aix-ppc64","DomainName":null,"AccountId":5,"AccountName":"builder","ApplicationID":null,"ApplicationDisplayName":null,"MaximumReleaseDuration":10079,"MaxReleaseDurationDays":6,"MaxReleaseDurationHours":23,"MaxReleaseDurationMinutes":59,"InstanceName":"","DefaultReleaseDuration":120,"DefaultReleaseDurationDays":0,"DefaultReleaseDurationHours":2,"DefaultReleaseDurationMinutes":0,"LastChangeDate":"2016-09-08T08:20:57.827","NextChangeDate":null,"IsChanging":false,"IsISAAccess":false,"PreferredNodeID":"ccce4d07-96e0-4ce5-ac2c-637c6995988f"},
 *
 * Output: ppManagedAccountsList. A link list of managed accounts
 */
DWORD
PbpsApiManagedAccountsParse(PSTR pszManagedAccount,
                            PLW_DLINKED_LIST *ppManagedAccountsList)
{
   DWORD dwError = LW_ERROR_SUCCESS;
   DWORD dwCount = 0;
   DWORD dwPlatformId = 0;
   DWORD dwSystemId = 0;
   DWORD dwAccountId = 0;
   PSTR  pszSystemName = NULL;
   PSTR  pszDomainName = NULL;
   PSTR  pszAccountName = NULL;
   PSTR  savePtr = NULL;
   PSTR  savePtr2 = NULL;
   PSTR  name = NULL;
   PSTR  value = NULL;
   PSTR  token = NULL;
   PSTR  pszEndPtr = NULL;
   PbpsApiManagedAccount_t* pAccount = NULL;
   PLW_DLINKED_LIST pTmpManagedAccountsList = NULL;

   // First tokenize based on  ",". The token is name:value pair
   // example {"PlatformID":4). Then subtoken based on ":". Next
   // strip off unwanted leading or trailing characters.

   token = strtok_r(pszManagedAccount, ",", &savePtr);
   while (token != NULL)
   {
      name = NULL;
      value = NULL;
      savePtr2 = NULL;

      name = strtok_r (token, ":", &savePtr2);
      if (name != NULL)
      {
         value = strtok_r(NULL, ":", &savePtr2);

         LwStripLeadingCharacters(name, '[');
         LwStripLeadingCharacters(name, '{');
         LwStripLeadingCharacters(name, '"');
         LwStripTrailingCharacters(name, '"');

         if (value != NULL)
         {
            LwStripTrailingCharacters(value, ']');
            LwStripTrailingCharacters(value, '}');
            LwStripTrailingCharacters(value, '"');
            LwStripLeadingCharacters(value, '"');
         }

         if (strncmp("PlatformID", name, 10) == 0)
         {
            dwPlatformId = (DWORD) LwStrtoll(value, &pszEndPtr, 10);
            dwCount++;
         }
         else if (strncmp("SystemId", name, 8) == 0)
         {
            dwSystemId = (DWORD) LwStrtoll(value, &pszEndPtr, 10);
            dwCount++;
         }
         else if (strncmp("SystemName", name, 10) == 0)
         {
            LwAllocateString(value, &pszSystemName);
            dwCount++;
         }
         else if (strncmp("DomainName", name, 10) == 0)
         {
            LwAllocateString(value, &pszDomainName);
            dwCount++;
         }
         else if (strncmp("AccountId", name, 8) == 0)
         {
            dwAccountId = (DWORD) LwStrtoll(value, &pszEndPtr, 10);
            dwCount++;
         }
         else if (strncmp("AccountName", name, 10) == 0)
         {
            LwAllocateString(value, &pszAccountName);
            LwStrToLower(pszAccountName);
            dwCount++;
         }

         if (dwCount == 6)
         {
            // We have the necessary values to fill-in a struct
            dwError = LwAllocateMemory(sizeof(PbpsApiManagedAccount_t),
                                       (PVOID*) &pAccount);
            BAIL_ON_LW_ERROR(dwError);

            pAccount->dwPlatformId = dwPlatformId;
            pAccount->dwSystemId = dwSystemId;
            pAccount->dwAccountId = dwAccountId;
            pAccount->pszSystemName = pszSystemName;
            pAccount->pszDomainName = pszDomainName;
            pAccount->pszAccountName = pszAccountName;

            // if the domain is supplied construct domain account name
            // to simplify searching
            // this intentionally filters out 'null'
            pAccount->pszDomainAccountName = NULL;
            if (pAccount->pszDomainName
                    && (!((strlen(pAccount->pszDomainName) == 4)
                            && strncmp(pAccount->pszDomainName, "null", 4) == 0))) {
                dwError = LwAllocateStringPrintf(
                     &(pAccount->pszDomainAccountName), "%s\\\\%s",
                        pAccount->pszDomainName,
                        pAccount->pszAccountName);
                BAIL_ON_LW_ERROR(dwError);
            } else {
                LwAllocateString(pAccount->pszAccountName, &pAccount->pszDomainAccountName);
            }

            dwError = LwDLinkedListAppend(&pTmpManagedAccountsList,
                                          pAccount);
            BAIL_ON_LW_ERROR(dwError);
            dwCount = 0;
         }
      }

      token = strtok_r(NULL, ",", &savePtr);
   }

   *ppManagedAccountsList = pTmpManagedAccountsList;

cleanup:
   return dwError;

error:
   if (pTmpManagedAccountsList)
     PbpsManagedAccountListFree(pTmpManagedAccountsList);

   goto cleanup;
}


/*
 * Copy managed account. This function allocates memory which must
 * be released using PbpsApiManagedAccountFree()
 *
 */
static
DWORD PbpsApiAccountCopy(PbpsApiManagedAccount_t *pSrcAccount,
                         PbpsApiManagedAccount_t **ppDestAccount)
{
   DWORD dwError = LW_ERROR_SUCCESS;
   PbpsApiManagedAccount_t *pTmp = NULL;

   dwError = LwAllocateMemory(sizeof(PbpsApiManagedAccount_t),
                                    (PVOID*) &pTmp);
   BAIL_ON_LW_ERROR(dwError);

   pTmp->dwPlatformId = pSrcAccount->dwPlatformId;
   pTmp->dwSystemId   = pSrcAccount->dwSystemId;
   pTmp->dwAccountId  = pSrcAccount->dwAccountId;
   pTmp->pszSystemName = NULL;
   pTmp->pszDomainName = NULL;
   pTmp->pszAccountName = NULL;
   pTmp->pszDomainAccountName = NULL;

   dwError = LwAllocateStringPrintf(
                     &(pTmp->pszSystemName), "%s",
                     pSrcAccount->pszSystemName);
   BAIL_ON_LW_ERROR(dwError);

   dwError = LwAllocateStringPrintf(
                     &(pTmp->pszAccountName), "%s",
                     pSrcAccount->pszAccountName);
   BAIL_ON_LW_ERROR(dwError);

   if (pSrcAccount->pszDomainName)
   {
      dwError = LwAllocateStringPrintf(
                        &(pTmp->pszDomainName), "%s",
                        pSrcAccount->pszDomainName);
      BAIL_ON_LW_ERROR(dwError);
   }

   if (pSrcAccount->pszDomainAccountName)
   {
      dwError = LwAllocateStringPrintf(
                        &(pTmp->pszDomainAccountName), "%s",
                        pSrcAccount->pszDomainAccountName);
      BAIL_ON_LW_ERROR(dwError);
   }

cleanup:
   *ppDestAccount = pTmp;
   return dwError;

error:
    PbpsApiManagedAccountFree(pTmp);
    pTmp = NULL;

  goto cleanup;
}

typedef struct PbpsApiSearchFor_s
{
  PSTR pszHostname;
  PSTR pszJoinAccount;
  BOOLEAN bFoundMatch;
  PbpsApiManagedAccount_t *pJoinAccount;
} PbpsApiSearchFor_t;

/*
 * Search link list of managed accounts. If a match is found, pUserData
 * is updated with a newly allocated PbpsApiManagedAccount_t. As
 * such, the caller will need to free the memory.
 * Full match is based on SystemName and optionally Domain and AccountName.
 * Failing a full match then only consider Domain\\AccountName.
 *
 */
static
VOID
PbpsApiManagedAccountSearch(PVOID pItem,
                            PVOID pUserData)
{
   DWORD dwError = LW_ERROR_SUCCESS;
   PbpsApiManagedAccount_t *pAccount = (PbpsApiManagedAccount_t *) pItem;
   PbpsApiSearchFor_t *needle = (PbpsApiSearchFor_t*) pUserData;
   PbpsApiManagedAccount_t *pDestAccount = NULL;

   if (needle->bFoundMatch)
   {
      // Found an exact match of SystemName and DomainAccountName || AccountName
      goto cleanup;
   }

   if ((strcasecmp(needle->pszHostname, pAccount->pszSystemName) == 0)
        && ((pAccount->pszDomainAccountName && (strcasecmp(needle->pszJoinAccount, pAccount->pszDomainAccountName) == 0))
           ||  (strcasecmp(needle->pszJoinAccount, pAccount->pszAccountName) == 0)))
   {
       //Allocate and copy pAccount into needle->pJoinAccount
       dwError = PbpsApiAccountCopy(pAccount, &pDestAccount);
       BAIL_ON_LW_ERROR(dwError);

       if (needle->pJoinAccount != NULL)
       {
          PbpsApiManagedAccountFree(needle->pJoinAccount);
          needle->pJoinAccount = NULL;
       }

       needle->pJoinAccount = pDestAccount;

       // No need to keep looking since we've found an entry which
       // meets all the criteria.
       needle->bFoundMatch = TRUE;

       DJ_LOG_VERBOSE("PbpsApiManagedAccountSearch found system %s with account %s",
                      needle->pszHostname,
                      needle->pszJoinAccount);
   }
   else if ((pAccount->pszDomainAccountName && (strcasecmp(needle->pszJoinAccount, pAccount->pszDomainAccountName) == 0))
           ||  (strcasecmp(needle->pszJoinAccount, pAccount->pszAccountName) == 0))
   {
      if (needle->pJoinAccount == NULL)
      {
         // Grab the first entry in the link list matching accountName
         // or accountNameFull. But keep looking for another entry
         // with matching SystemName.
         // Allocate and copy pAccount into needle->pJoinAccount
         dwError = PbpsApiAccountCopy(pAccount, &pDestAccount);
         BAIL_ON_LW_ERROR(dwError);
         needle->pJoinAccount = pDestAccount;

         DJ_LOG_VERBOSE("PbpsApiManagedAccountSearch found account %s",
                         needle->pszJoinAccount);
      }
   }

cleanup:
   return;

error:
   goto cleanup;
}


/*
 *
 * Search list of managed accounts for the given name.
 * Returned account must be freed with PbpsApiManagedAccountFree().
 *
 */
DWORD
PbpsApiGetJoinAccount(
           PbpsApi_t *pApi,
           PSTR pszJoinAccount,
           PbpsApiManagedAccount_t **ppAccount)
{
   DWORD dwError = LW_ERROR_SUCCESS;
   PbpsApiSearchFor_t needle;

   needle.bFoundMatch = FALSE;
   needle.pszJoinAccount = pszJoinAccount;
   needle.pszHostname = NULL;
   needle.pJoinAccount = NULL;

   dwError = DJGetComputerName(&needle.pszHostname);
   BAIL_ON_LW_ERROR(dwError);

   LwDLinkedListForEach(pApi->session.pManagedAccountList,
                      &PbpsApiManagedAccountSearch,
                      (PVOID) &needle);

   if (needle.pJoinAccount)
     *ppAccount = needle.pJoinAccount;
   else
   {
      dwError = LW_ERROR_NO_SUCH_USER;
      BAIL_ON_LW_ERROR(dwError);
   }

cleanup:

   LW_SAFE_FREE_STRING(needle.pszHostname);

   return dwError;
error:
   *ppAccount = NULL;
   goto cleanup;
}

