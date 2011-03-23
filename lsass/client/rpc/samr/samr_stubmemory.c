/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright Likewise Software    2004-2008
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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        samr_stubmemory.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        Samr DCE/RPC stub memory cleanup functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


void
SamrCleanStubRidNameArray(
    RID_NAME_ARRAY *pRidNames
    )
{
    unsigned32 rpcStatus = 0;
    UINT32 i = 0;

    for (i = 0; i < pRidNames->dwCount; i++) {
        PRID_NAME pRidName = &(pRidNames->pEntries[i]);
        rpc_sm_client_free(pRidName->Name.Buffer, &rpcStatus);
    }

    rpc_sm_client_free(pRidNames->pEntries, &rpcStatus);
}


void
SamrFreeStubRidNameArray(
    RID_NAME_ARRAY *pRidNames)
{
    unsigned32 rpcStatus = RPC_S_OK;

    SamrCleanStubRidNameArray(pRidNames);
    rpc_sm_client_free(pRidNames, &rpcStatus);
}


void
SamrCleanStubIds(
    IDS *pIds
    )
{
    unsigned32 rpcStatus = RPC_S_OK;

    if (pIds->dwCount) {
        rpc_sm_client_free(pIds->pIds, &rpcStatus);
    }
}


void
SamrCleanStubUnicodeStringArray(
    UNICODE_STRING_ARRAY *pUniStrings
    )
{
    unsigned32 rpcStatus = RPC_S_OK;
    UINT32 i = 0;

    for (i = 0; i < pUniStrings->dwCount; i++) {
        PUNICODE_STRING pUniString = &(pUniStrings->pNames[i]);
        rpc_sm_client_free(pUniString->Buffer, &rpcStatus);
    }

    rpc_sm_client_free(pUniStrings->pNames, &rpcStatus);
}


void
SamrCleanStubEntryArray(
    ENTRY_ARRAY *pEntries
    )
{
    unsigned32 rpcStatus = RPC_S_OK;
    UINT32 i = 0;

    for (i = 0; i < pEntries->dwCount; i++) {
        PENTRY pEntry = &(pEntries->pEntries[i]);
        rpc_sm_client_free(pEntry->Name.Buffer, &rpcStatus);
    }

    rpc_sm_client_free(pEntries->pEntries, &rpcStatus);
}


void
SamrFreeStubEntryArray(
    ENTRY_ARRAY *pEntries
    )
{
    unsigned32 rpcStatus = RPC_S_OK;

    SamrCleanStubEntryArray(pEntries);
    rpc_sm_client_free(pEntries, &rpcStatus);
}


void
SamrFreeStubDomSid(
    PSID pSid
    )
{
    unsigned32 rpcStatus = RPC_S_OK;

    rpc_sm_client_free(pSid, &rpcStatus);
}


void
SamrCleanStubSidArray(
    SID_ARRAY *pSids
    )
{
    unsigned32 rpcStatus = RPC_S_OK;
    UINT32 i = 0;

    for (i = 0; i < pSids->dwNumSids; i++) {
        PSID pSid = pSids->pSids[i].pSid;
        rpc_sm_client_free(pSid, &rpcStatus);
    }

    rpc_sm_client_free(pSids->pSids, &rpcStatus);
}


void
SamrCleanStubRidWithAttributeArray(
    RID_WITH_ATTRIBUTE_ARRAY *pRidAttribs)
{
    unsigned32 rpcStatus = RPC_S_OK;

    rpc_sm_client_free(pRidAttribs->pRids, &rpcStatus);
}


void
SamrFreeStubRidWithAttributeArray(
    RID_WITH_ATTRIBUTE_ARRAY *pRidAttribs)
{
    unsigned32 rpcStatus = RPC_S_OK;

    SamrCleanStubRidWithAttributeArray(pRidAttribs);
    rpc_sm_client_free(pRidAttribs, &rpcStatus);
}


void
SamrCleanStubAliasInfo(
    AliasInfo *pInfo,
    UINT16     Level
    )
{
    unsigned32 rpcStatus = RPC_S_OK;

    switch (Level) {
    case ALIAS_INFO_ALL:
        rpc_sm_client_free(pInfo->all.name.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->all.description.Buffer, &rpcStatus);
        break;

    case ALIAS_INFO_NAME:
        rpc_sm_client_free(pInfo->name.Buffer, &rpcStatus);
        break;

    case ALIAS_INFO_DESCRIPTION:
        rpc_sm_client_free(pInfo->description.Buffer, &rpcStatus);
        break;
    }
}


void
SamrFreeStubAliasInfo(
    AliasInfo *pInfo,
    UINT16     Level
    )
{
    unsigned32 rpcStatus = RPC_S_OK;

    SamrCleanStubAliasInfo(pInfo, Level);
    rpc_sm_client_free(pInfo, &rpcStatus);
}


void
SamrCleanStubDomainInfo(
    DomainInfo *pInfo,
    UINT16      Level)
{
    unsigned32 rpcStatus = RPC_S_OK;

    switch (Level) {
    case 2:
        rpc_sm_client_free(pInfo->info2.comment.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info2.domain_name.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info2.primary.Buffer, &rpcStatus);
        break;

    case 4:
        rpc_sm_client_free(pInfo->info4.comment.Buffer, &rpcStatus);
        break;

    case 5:
        rpc_sm_client_free(pInfo->info5.domain_name.Buffer, &rpcStatus);
        break;

    case 6:
        rpc_sm_client_free(pInfo->info6.primary.Buffer, &rpcStatus);
        break;

    case 11:
        SamrCleanStubDomainInfo(pInfo, 2);
    }
}


void
SamrFreeStubDomainInfo(
    DomainInfo *pInfo,
    UINT16      Level)
{
    unsigned32 rpcStatus = RPC_S_OK;

    SamrCleanStubDomainInfo(pInfo, Level);
    rpc_sm_client_free(pInfo, &rpcStatus);
}


void
SamrCleanStubUserInfo(
    UserInfo *pInfo,
    UINT16    Level
    )
{
    unsigned32 rpcStatus = RPC_S_OK;

    switch (Level) {
    case 1:
        rpc_sm_client_free(pInfo->info1.account_name.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info1.full_name.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info1.description.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info1.comment.Buffer, &rpcStatus);
        break;

    case 2:
        rpc_sm_client_free(pInfo->info2.comment.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info2.unknown1.Buffer, &rpcStatus);
        break;

    case 3:
        rpc_sm_client_free(pInfo->info3.account_name.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info3.full_name.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info3.home_directory.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info3.home_drive.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info3.logon_script.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info3.profile_path.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info3.workstations.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info3.logon_hours.units, &rpcStatus);
        break;

    case 4:
        rpc_sm_client_free(pInfo->info4.logon_hours.units, &rpcStatus);
        break;

    case 5:
        rpc_sm_client_free(pInfo->info5.account_name.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info5.full_name.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info5.home_directory.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info5.home_drive.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info5.logon_script.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info5.profile_path.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info5.description.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info5.workstations.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info5.logon_hours.units, &rpcStatus);
        break;

    case 6:
        rpc_sm_client_free(pInfo->info6.account_name.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info6.full_name.Buffer, &rpcStatus);
        break;

    case 7:
        rpc_sm_client_free(pInfo->info7.account_name.Buffer, &rpcStatus);
        break;

    case 8:
        rpc_sm_client_free(pInfo->info8.full_name.Buffer, &rpcStatus);
        break;

    case 10:
        rpc_sm_client_free(pInfo->info10.home_directory.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info10.home_drive.Buffer, &rpcStatus);
        break;

    case 11:
        rpc_sm_client_free(pInfo->info11.logon_script.Buffer, &rpcStatus);
        break;

    case 12:
        rpc_sm_client_free(pInfo->info12.profile_path.Buffer, &rpcStatus);
        break;

    case 13:
        rpc_sm_client_free(pInfo->info13.description.Buffer, &rpcStatus);
        break;

    case 14:
        rpc_sm_client_free(pInfo->info14.workstations.Buffer, &rpcStatus);
        break;

    case 20:
        rpc_sm_client_free(pInfo->info20.parameters.Buffer, &rpcStatus);
        break;

    case 21:
        rpc_sm_client_free(pInfo->info21.account_name.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info21.full_name.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info21.home_directory.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info21.home_drive.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info21.logon_script.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info21.profile_path.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info21.description.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info21.workstations.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info21.comment.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info21.parameters.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info21.unknown1.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info21.unknown2.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info21.unknown3.Buffer, &rpcStatus);
        if (&pInfo->info21.buf_count) {
            rpc_sm_client_free(pInfo->info21.buffer, &rpcStatus);
        }
        rpc_sm_client_free(pInfo->info21.logon_hours.units, &rpcStatus);
        break;

    case 23:
        rpc_sm_client_free(pInfo->info23.info.account_name.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info23.info.full_name.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info23.info.home_directory.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info23.info.home_drive.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info23.info.logon_script.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info23.info.profile_path.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info23.info.description.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info23.info.workstations.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info23.info.comment.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info23.info.parameters.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info23.info.unknown1.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info23.info.unknown2.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info23.info.unknown3.Buffer, &rpcStatus);
        if (&pInfo->info23.info.buf_count) {
            rpc_sm_client_free(pInfo->info23.info.buffer, &rpcStatus);
        }
        rpc_sm_client_free(pInfo->info23.info.logon_hours.units, &rpcStatus);

        break;

    case 25:
        rpc_sm_client_free(pInfo->info25.info.account_name.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info25.info.full_name.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info25.info.home_directory.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info25.info.home_drive.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info25.info.logon_script.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info25.info.profile_path.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info25.info.description.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info25.info.workstations.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info25.info.comment.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info25.info.parameters.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info25.info.unknown1.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info25.info.unknown2.Buffer, &rpcStatus);
        rpc_sm_client_free(pInfo->info25.info.unknown3.Buffer, &rpcStatus);
        if (&pInfo->info25.info.buf_count) {
            rpc_sm_client_free(pInfo->info25.info.buffer, &rpcStatus);
        }
        rpc_sm_client_free(pInfo->info25.info.logon_hours.units, &rpcStatus);

        break;
    }
}


void
SamrFreeStubUserInfo(
    UserInfo *pInfo,
    UINT16    Level
    )
{
    unsigned32 rpcStatus = RPC_S_OK;

    SamrCleanStubUserInfo(pInfo, Level);
    rpc_sm_client_free(pInfo, &rpcStatus);
}


static
void
SamrCleanStubDisplayInfoFull(
    SamrDisplayInfoFull *pInfo
    )
{
    unsigned32 rpcStatus = RPC_S_OK;
    UINT32 i = 0;

    for (i = 0; i < pInfo->count; i++) {
        SamrDisplayEntryFull *pEntry = &(pInfo->entries[i]);

        rpc_sm_client_free(pEntry->account_name.Buffer, &rpcStatus);
        rpc_sm_client_free(pEntry->description.Buffer, &rpcStatus);
        rpc_sm_client_free(pEntry->full_name.Buffer, &rpcStatus);
    }

    rpc_sm_client_free(pInfo->entries, &rpcStatus);
}


static
void
SamrCleanStubDisplayInfoGeneral(
    SamrDisplayInfoGeneral *pInfo
    )
{
    unsigned32 rpcStatus = RPC_S_OK;
    UINT32 i = 0;

    for (i = 0; i < pInfo->count; i++) {
        SamrDisplayEntryGeneral *pEntry = &(pInfo->entries[i]);

        rpc_sm_client_free(pEntry->account_name.Buffer, &rpcStatus);
        rpc_sm_client_free(pEntry->description.Buffer, &rpcStatus);
    }

    rpc_sm_client_free(pInfo->entries, &rpcStatus);
}


static
void
SamrCleanStubDisplayInfoGeneralGroups(
    SamrDisplayInfoGeneralGroups *pInfo
    )
{
    unsigned32 rpcStatus = RPC_S_OK;
    UINT32 i = 0;

    for (i = 0; i < pInfo->count; i++) {
        SamrDisplayEntryGeneralGroup *pEntry = &(pInfo->entries[i]);

        rpc_sm_client_free(pEntry->account_name.Buffer, &rpcStatus);
        rpc_sm_client_free(pEntry->description.Buffer, &rpcStatus);
    }

    rpc_sm_client_free(pInfo->entries, &rpcStatus);
}


static
void
SamrCleanStubDisplayInfoAscii(
    SamrDisplayInfoAscii *pInfo
    )
{
    unsigned32 rpcStatus = RPC_S_OK;
    UINT32 i = 0;

    for (i = 0; i < pInfo->count; i++) {
        SamrDisplayEntryAscii *pEntry = &(pInfo->entries[i]);

        if (pEntry->account_name.Buffer) {
            rpc_sm_client_free(pEntry->account_name.Buffer, &rpcStatus);
        }
    }

    rpc_sm_client_free(pInfo->entries, &rpcStatus);
}


void
SamrCleanStubDisplayInfo(
    SamrDisplayInfo *pInfo,
    UINT16           Level
    )
{
    switch (Level) {
    case 1:
        SamrCleanStubDisplayInfoFull(&pInfo->info1);
        break;

    case 2:
        SamrCleanStubDisplayInfoGeneral(&pInfo->info2);
        break;

    case 3:
        SamrCleanStubDisplayInfoGeneralGroups(&pInfo->info3);
        break;

    case 4:
        SamrCleanStubDisplayInfoAscii(&pInfo->info4);
        break;

    case 5:
        SamrCleanStubDisplayInfoAscii(&pInfo->info5);
        break;
    }
}


void
SamrCleanStubSecurityDescriptorBuffer(
    PSAMR_SECURITY_DESCRIPTOR_BUFFER pSecDescBuffer
    )
{
    unsigned32 rpcStatus = 0;
    rpc_sm_client_free(pSecDescBuffer->pBuffer,
                       &rpcStatus);
}


void
SamrFreeStubSecurityDescriptorBuffer(
    PSAMR_SECURITY_DESCRIPTOR_BUFFER pSecDescBuffer
    )
{
    unsigned32 rpcStatus = 0;

    SamrCleanStubSecurityDescriptorBuffer(pSecDescBuffer);
    rpc_sm_client_free(pSecDescBuffer, &rpcStatus);
}
