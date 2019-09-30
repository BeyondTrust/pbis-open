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
 *        globals.c
 *
 * Abstract:
 *
 *        BeyondTrust Task System (LWTASK)
 *
 *        Utilities
 *
 *        Globals
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#include "includes.h"

// Note: Be careful about the items you put here
//       This library can be exercised through
//       threaded components.

//
// These variables are protected in MT-SAFE components
// if LW_ENABLE_THREADS is enabled when compiling those
// components. Also use liblwtasklog_r.so
//
HANDLE                  ghLwTaskLog        = (HANDLE)NULL;
LW_TASK_LOG_LEVEL       gLwTaskMaxLogLevel = LW_TASK_LOG_LEVEL_ERROR;
LW_TASK_LOG_TARGET      gLWTASK_LOG_TARGET = LW_TASK_LOG_TARGET_DISABLED;
PFN_LW_TASK_LOG_MESSAGE gpfnLwTaskLogger   = NULL;
