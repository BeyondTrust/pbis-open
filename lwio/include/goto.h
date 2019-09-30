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
 *        goto.h
 *
 * Abstract:
 *
 *        GOTO_CLEANUP<XXX> macros used to bail.  Logging is done separately
 *        by using a local EE (early exit) variable (of type int),
 *        which contains the line nubmer of the GOTO_CLEANUP<XXX>_EE
 *        operation.
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#ifndef __GOTO_H__
#define __GOTO_H__

#define GOTO_CLEANUP() \
  do { goto cleanup; } while (0)

#define GOTO_CLEANUP_EE(EE) \
  do { (EE) = __LINE__; goto cleanup; } while (0)

#define _GOTO_CLEANUP_ON_NONZERO(value) \
    do { if (value) goto cleanup; } while (0)

#define _GOTO_CLEANUP_ON_NONZERO_EE(value, EE) \
      do { if (value) { (EE) = __LINE__; goto cleanup; } } while (0)

#define GOTO_CLEANUP_ON_STATUS(status) \
    _GOTO_CLEANUP_ON_NONZERO(status)

#define GOTO_CLEANUP_ON_STATUS_EE(status, EE) \
    _GOTO_CLEANUP_ON_NONZERO_EE(status, EE)

#define GOTO_CLEANUP_ON_ERRNO(error) \
    _GOTO_CLEANUP_ON_NONZERO(error)

#define GOTO_CLEANUP_ON_ERRNO_EE(error, EE) \
    _GOTO_CLEANUP_ON_NONZERO_EE(error, EE)

#endif /* __GOTO_H__ */
