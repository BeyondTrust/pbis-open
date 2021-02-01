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
 *        iotestctl.h
 *
 * Abstract:
 *
 *        IO Test Driver Control Codes and Such
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#ifndef __IOTEST_CTL_H__
#define __IOTEST_CTL_H__

#include <lwio/io-types.h>

#define _IOTEST_IOCTL_BASE 0
#define _IOTEST_IOCTL(x) (_IOTEST_IOCTL_BASE + (x))

#define IOTEST_IOCTL_ECHO                       _IOTEST_IOCTL(1)
#define IOTEST_IOCTL_ADD                        _IOTEST_IOCTL(2)
#define IOTEST_IOCTL_TEST_SYNC_CREATE           _IOTEST_IOCTL(3)
#define IOTEST_IOCTL_TEST_ASYNC_CREATE          _IOTEST_IOCTL(4)
#define IOTEST_IOCTL_TEST_RUNDOWN               _IOTEST_IOCTL(5)
#define IOTEST_IOCTL_TEST_SLEEP                 _IOTEST_IOCTL(6)

// IOTEST_IOCTL_ECHO
// IN: Buffer
// OUT: Buffer

// IOTEST_IOCTL_ADD
// IN: IOTEST_INBUFFER_IOCTL_ADD
// OUT: IOTEST_OUTBUFFER_IOCTL_ADD

typedef struct _IOTEST_INBUFFER_IOCTL_ADD {
    LONG Operand1;
    LONG Operand2;
} IOTEST_INBUFFER_IOCTL_ADD, *PIOTEST_INBUFFER_IOCTL_ADD;

typedef struct _IOTEST_OUTBUFFER_IOCTL_ADD {
    LONG Result;
} IOTEST_OUTBUFFER_IOCTL_ADD, *PIOTEST_OUTBUFFER_IOCTL_ADD;

// IOTEST_IOCTL_TEST_SYNC_CREATE
// IN: N/A
// OUT: N/A

// IOTEST_IOCTL_TEST_ASYNC_CREATE
// IN: N/A
// OUT: N/A

// IOTEST_IOCTL_TEST_SHUTDOWN
// IN: N/A
// OUT: N/A

// IOTEST_IOCTL_TEST_SLEEP
// IN: N/A
// OUT: N/A

#endif /* __IOTEST_CTL_H__ */
