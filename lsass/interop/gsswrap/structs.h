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
 *        structs.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
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

