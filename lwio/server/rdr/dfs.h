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

/*
 * Module Name:
 *
 *        dfs.h
 *
 * Abstract:
 *
 *        LWIO Redirector
 *
 *        Common DFS code
 *
 * Author: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#ifndef __RDR_DFS_H__
#define __RDR_DFS_H__

#define DFS_TARGET_NON_ROOT 0
#define DFS_TARGET_ROOT 1

#define DFS_MAX_RESPONSE_SIZE 8192

typedef struct _DFS_REQUEST_HEADER
{
    USHORT usMaxReferralLevel;
    /* Path follows */
} __attribute__((__packed__))
DFS_REQUEST_HEADER, *PDFS_REQUEST_HEADER;

typedef struct _DFS_RESPONSE_HEADER
{
    USHORT usPathConsumed;
    USHORT usNumReferrals;
    ULONG ulFlags;
    /* Referral entries follow */
} __attribute__((__packed__))
DFS_RESPONSE_HEADER, *PDFS_RESPONSE_HEADER;

typedef struct _DFS_REFERRAL_V4
{
    USHORT usVersionNumber;
    USHORT usSize;
    USHORT usServerType;
    USHORT usFlags;
    ULONG ulTimeToLive;
} __attribute__((__packed__))
DFS_REFERRAL_V4, *PDFS_REFERRAL_V4;

typedef struct _DFS_REFERRAL_V4_NORMAL
{
    DFS_REFERRAL_V4 Base;
    USHORT usDfsPathOffset;
    USHORT usDfsAlternatePathOffset;
    USHORT usNetworkAddressOffset;
    BYTE ServiceSiteGuid;
} __attribute__((__packed__))
DFS_REFERRAL_V4_NORMAL, *PDFS_REFERRAL_V4_NORMAL;

typedef struct _RDR_DFS_REFERRAL
{
    unsigned bIsRoot:1;
    PWSTR pwszReferral;
} RDR_DFS_REFERRAL, *PRDR_DFS_REFERRAL;

typedef struct _RDR_DFS_NAMESPACE
{
    /* FIXME: use a trie or something instead of a linked list */
    LW_LIST_LINKS Link;
    PWSTR pwszNamespace;
    ULONG ulExpirationTime;
    USHORT usReferralCount;
    PRDR_DFS_REFERRAL pReferrals;
} RDR_DFS_NAMESPACE, *PRDR_DFS_NAMESPACE;

NTSTATUS
RdrDfsResolvePath(
    PCWSTR pwszPath,
    USHORT usTry,
    PWSTR* ppwszResolved,
    PBOOLEAN pbIsRoot
    );

NTSTATUS
RdrDfsRegisterNamespace(
    PCWSTR pwszPath,
    PDFS_RESPONSE_HEADER pResponse,
    ULONG ulResponseSize
    );

NTSTATUS
RdrDfsChaseReferral1(
    PRDR_OP_CONTEXT pContext,
    PRDR_TREE pTree
    );

NTSTATUS
RdrDfsChaseReferral2(
    PRDR_OP_CONTEXT pContext,
    PRDR_TREE2 pTree
    );

NTSTATUS
RdrDfsConnect(
    IN OPTIONAL PRDR_SOCKET pSocket,
    IN PUNICODE_STRING pPath,
    IN PIO_CREDS pCreds,
    IN uid_t Uid,
    IN NTSTATUS lastError,
    IN OUT PUSHORT pusTry,
    OUT PWSTR* ppwszFilePath,
    OUT PWSTR* ppwszCanonicalPath,
    IN PRDR_OP_CONTEXT pContinue
    );

NTSTATUS
RdrDfsConnectAttempt(
    PRDR_OP_CONTEXT pContext
    );

#endif
