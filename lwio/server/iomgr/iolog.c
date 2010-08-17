/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        iolog.c
 *
 * Abstract:
 *
 *        IO Manager Logging Utility Functions Implementation
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include "iop.h"

#define CASE_ASSIGN_STRING(Variable, Value) case Value: (Variable) = #Value; break

//
// Driver-specific
//

PCSTR
IoGetIrpTypeString(
    IN IRP_TYPE Type
    )
{
    PCSTR result = NULL;

    switch (Type)
    {
    CASE_ASSIGN_STRING(result, IRP_TYPE_UNINITIALIZED);
    CASE_ASSIGN_STRING(result, IRP_TYPE_CREATE);
    CASE_ASSIGN_STRING(result, IRP_TYPE_CLOSE);
    CASE_ASSIGN_STRING(result, IRP_TYPE_READ);
    CASE_ASSIGN_STRING(result, IRP_TYPE_WRITE);
    CASE_ASSIGN_STRING(result, IRP_TYPE_DEVICE_IO_CONTROL);
    CASE_ASSIGN_STRING(result, IRP_TYPE_FS_CONTROL);
    CASE_ASSIGN_STRING(result, IRP_TYPE_FLUSH_BUFFERS);
    CASE_ASSIGN_STRING(result, IRP_TYPE_QUERY_INFORMATION);
    CASE_ASSIGN_STRING(result, IRP_TYPE_SET_INFORMATION);
    CASE_ASSIGN_STRING(result, IRP_TYPE_CREATE_NAMED_PIPE);
    CASE_ASSIGN_STRING(result, IRP_TYPE_CREATE_MAILSLOT);
    CASE_ASSIGN_STRING(result, IRP_TYPE_QUERY_DIRECTORY);
    CASE_ASSIGN_STRING(result, IRP_TYPE_QUERY_VOLUME_INFORMATION);
    CASE_ASSIGN_STRING(result, IRP_TYPE_SET_VOLUME_INFORMATION);
    CASE_ASSIGN_STRING(result, IRP_TYPE_LOCK_CONTROL);
    CASE_ASSIGN_STRING(result, IRP_TYPE_QUERY_SECURITY);
    CASE_ASSIGN_STRING(result, IRP_TYPE_SET_SECURITY);
    CASE_ASSIGN_STRING(result, IRP_TYPE_QUERY_QUOTA);
    CASE_ASSIGN_STRING(result, IRP_TYPE_SET_QUOTA);
    default:
        result = "*Unknown*";
    }

    return result;
}
