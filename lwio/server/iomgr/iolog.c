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

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
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
