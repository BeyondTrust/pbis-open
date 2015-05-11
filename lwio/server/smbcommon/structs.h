/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
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

