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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        structs.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        GSSAPI Wrappers
 *
 *        Structures
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

typedef enum
{
    LSA_GSS_CONTEXT_STATUS_INIT = 0,
    LSA_GSS_CONTEXT_STATUS_IN_PROGRESS,
    LSA_GSS_CONTEXT_STATUS_COMPLETE,
    LSA_GSS_CONTEXT_STATUS_ERROR

} LSA_GSS_CONTEXT_STATUS;

typedef struct _LSA_GSS_ERROR_INFO
{
    OM_uint32 dwMajorStatus;
    OM_uint32 dwMinorStatus;

} LSA_GSS_ERROR_INFO, *PLSA_GSS_ERROR_INFO;

typedef struct _LSA_GSS_CONTEXT
{
    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    gss_ctx_id_t     gssCtx;

    LSA_GSS_ERROR_INFO errorInfo;

    LSA_GSS_CONTEXT_STATUS status;

} LSA_GSS_CONTEXT, *PLSA_GSS_CONTEXT;

typedef struct _LSA_GSS_GLOBALS
{
    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    BOOLEAN bLwMapSecurityInitialized;

    PLW_MAP_SECURITY_CONTEXT pSecurityContext;

} LSA_GSS_GLOBALS, *PLSA_GSS_GLOBALS;

