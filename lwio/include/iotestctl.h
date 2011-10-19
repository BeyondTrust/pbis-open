/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
