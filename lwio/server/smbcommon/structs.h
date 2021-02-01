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

#ifndef __SMBCOMMON_STRUCTS_H__
#define __SMBCOMMON_STRUCTS_H__


/*
   Based on RFC2478

   MechType ::= OBJECT IDENTIFIER

   MechTypeList ::= SEQUENCE OF MechType

   ContextFlags ::= BIT STRING
                    {
                         delegFlag  (0),
                         mutualFlag (1),
                         replayFlag (2),
                         sequenceFlag (3),
                         anonFlag     (4),
                         confFlag     (5),
                         integFlag    (6)
                    }

   NegTokenInit ::= SEQUENCE OF
                    {
                         mechTypes   [0] MechTypeList OPTIONAL
                         reqFlags    [1] ContextFlags OPTIONAL
                         mechToken   [2] OCTET STRING OPTIONAL
                         mechListMIC [3] OCTET STRING OPTIONAL
                    }

 */

typedef struct __OID
{
    uint32_t dwLength;
    uint8_t* pIdList;
} OID, *POID;

typedef OID MECHTYPE, *PMECHTYPE;

typedef struct __MECHTYPELIST
{
    uint32_t dwLength;
    PMECHTYPE pMechTypeArray;
} MECHTYPELIST, *PMECHTYPELIST;

typedef struct __OCTET_STRING
{
    uint32_t dwLength;
    PBYTE    pData;
} OCTET_STRING, *POCTET_STRING;

typedef struct __CONTEXT_FLAGS
{
    uint32_t delegFlag    : 1;
    uint32_t mutualFlag   : 1;
    uint32_t replayFlag   : 1;
    uint32_t sequenceFlag : 1;
    uint32_t anonFlag     : 1;
    uint32_t confFlag     : 1;
    uint32_t integFlag    : 1;
} CONTEXT_FLAGS, *PCONTEXT_FLAGS;

typedef struct __NEGTOKENINIT
{
    PMECHTYPELIST pMechTypeList;
    PCONTEXT_FLAGS pContextFlags;
    POCTET_STRING pMechToken;
    POCTET_STRING pMechListMIC;
} NEGTOKENINIT, *PNEGTOKENINIT;

typedef struct __SMB_GSS_SEC_CONTEXT
{
    SMB_GSS_SEC_CONTEXT_STATE state;
    PCtxtHandle               pGSSContext;
    gss_name_t                target_name;
    gss_cred_id_t             credHandle;
} SMB_GSS_SEC_CONTEXT, *PSMB_GSS_SEC_CONTEXT;

#endif

