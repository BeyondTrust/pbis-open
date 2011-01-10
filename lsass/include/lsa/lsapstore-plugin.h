/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lsapstore-plugin.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Password Store Plugin Interface
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 *
 */

#ifndef __LSA_PSTORE_PLUGIN_H__
#define __LSA_PSTORE_PLUGIN_H__

#include <lsa/lsapstore-types.h>

//
// The plugin is only called when the machine password information
// for the default domain is changed or deleted.
//

typedef struct _LSA_PSTORE_PLUGIN_CONTEXT *PLSA_PSTORE_PLUGIN_CONTEXT;

typedef
VOID
(*LSA_PSTORE_PLUGIN_CLEANUP_FUNCTION)(
    IN PLSA_PSTORE_PLUGIN_CONTEXT Context
    );

typedef
DWORD
(*LSA_PSTORE_PLUGIN_SET_PASSWORD_INFO_FUNCTION)(
    IN PLSA_PSTORE_PLUGIN_CONTEXT Context,
    IN PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo
    );

typedef
DWORD
(*LSA_PSTORE_PLUGIN_DELETE_PASSWORD_INFO_FUNCTION)(
    IN PLSA_PSTORE_PLUGIN_CONTEXT Context
    );

typedef struct _LSA_PSTORE_PLUGIN_DISPATCH {
    LSA_PSTORE_PLUGIN_CLEANUP_FUNCTION Cleanup;
    LSA_PSTORE_PLUGIN_SET_PASSWORD_INFO_FUNCTION SetPasswordInfo;
    LSA_PSTORE_PLUGIN_DELETE_PASSWORD_INFO_FUNCTION DeletePasswordInfo;
} LSA_PSTORE_PLUGIN_DISPATCH, *PLSA_PSTORE_PLUGIN_DISPATCH;

#define _LSA_PSTORE_PLUGIN_VERSION 1
#define LSA_PSTORE_PLUGIN_VERSION \
    ((_LSA_PSTORE_PLUGIN_VERSION << 16) | (sizeof(LSA_PSTORE_PLUGIN_DISPATCH) & 0xFFFF))

#define LSA_PSTORE_PLUGIN_INITIALIZE_FUNCTION_NAME \
    "LsaPstorePluginInitializeContext"

typedef
DWORD
(*LSA_PSTORE_PLUGIN_INITIALIZE_FUNCTION)(
    IN ULONG Version,
    OUT PLSA_PSTORE_PLUGIN_DISPATCH* Dispatch,
    OUT PLSA_PSTORE_PLUGIN_CONTEXT* Context
    );

#endif
