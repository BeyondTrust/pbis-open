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
 *        lwtaskdef.h
 *
 * Abstract:
 *
 *        BeyondTrust Task System (LWTASK) Client/Server common definitions
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 * 
 */
#ifndef __LWTASKDEF_H__
#define __LWTASKDEF_H__

typedef enum
{
    LW_TASK_TYPE_UNKNOWN = 0,
    LW_TASK_TYPE_MIGRATE

} LW_TASK_TYPE;

typedef enum
{
    LW_TASK_ARG_TYPE_UNKNOWN = 0,
    LW_TASK_ARG_TYPE_STRING,
    LW_TASK_ARG_TYPE_STRING_MULTI_CSV,
    LW_TASK_ARG_TYPE_INTEGER,
    LW_TASK_ARG_TYPE_DATETIME,
    LW_TASK_ARG_TYPE_FLOAT,
    LW_TASK_ARG_TYPE_DOUBLE

} LW_TASK_ARG_TYPE;

typedef DWORD LW_TASK_ARG_FLAG;

#define LW_TASK_ARG_FLAG_NONE      0x00000000
#define LW_TASK_ARG_FLAG_MANDATORY 0x00000001
#define LW_TASK_ARG_FLAG_PERSIST   0x00000002

typedef struct _LW_TASK_STATUS
{
    DWORD dwError;

    DWORD dwPercentComplete;

    time_t startTime;
    time_t endTime;

} LW_TASK_STATUS, *PLW_TASK_STATUS;

typedef struct _LW_TASK_ARG
{
    PSTR  pszArgName;
    DWORD dwArgType;
    PSTR  pszArgValue;

} LW_TASK_ARG, *PLW_TASK_ARG;

typedef struct _LW_TASK_ARG_INFO
{
    PSTR             pszArgName;
    LW_TASK_ARG_TYPE argType;
    LW_TASK_ARG_FLAG dwFlags;

} LW_TASK_ARG_INFO, *PLW_TASK_ARG_INFO;

typedef struct _LW_TASK_INFO
{
    PSTR             pszTaskId;
    DWORD            dwNumArgs;
    PLW_TASK_ARG     pArgArray;
} LW_TASK_INFO, *PLW_TASK_INFO;

#endif /* __LWTASKDEF_H__ */
