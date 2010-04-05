/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
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
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwio-semaphore.h
 *
 * Abstract:
 *
 *        Likewise IO
 *
 *        Semaphore Code
 *
 * Author: Danilo Almeida (dalmeida@likewise.com)
 */

#ifndef __LWIO_SEMAPHORE_H__
#define __LWIO_SEMAPHORE_H__

#if defined(__LWI_DARWIN__)
typedef struct {
    pthread_mutex_t Mutex;
    pthread_cond_t Condition;
    DWORD Count;
} LSMB_SEMAPHORE, *PLSMB_SEMAPHORE;
#else
typedef sem_t LSMB_SEMAPHORE, *PLSMB_SEMAPHORE;
#endif

#if defined(__LWI_DARWIN__)
NTSTATUS
SMBSemaphoreInit(
    OUT PLSMB_SEMAPHORE pSemaphore,
    IN DWORD Count
    );

NTSTATUS
SMBSemaphoreWait(
    IN PLSMB_SEMAPHORE pSemaphore
    );

NTSTATUS
SMBSemaphorePost(
    IN PLSMB_SEMAPHORE pSemaphore
    );

VOID
SMBSemaphoreDestroy(
    IN OUT PLSMB_SEMAPHORE pSemaphore
    );
#else  /* __LWI_DARWIN__ */
#define _SMB_SEMAPHORE_SYSCALL(x) ((((x) < 0) && errno) ? LwErrnoToNtStatus(errno) : 0)

#define SMBSemaphoreInit(pSemaphore, Count) _SMB_SEMAPHORE_SYSCALL(sem_init(pSemaphore, 0, Count))
#define SMBSemaphoreWait(pSemaphore) _SMB_SEMAPHORE_SYSCALL(sem_wait(pSemaphore))
#define SMBSemaphorePost(pSemaphore) _SMB_SEMAPHORE_SYSCALL(sem_post(pSemaphore))
#define SMBSemaphoreDestroy(pSemaphore) \
    do { \
        int localError = _SMB_SEMAPHORE_SYSCALL(sem_destroy(pSemaphore)); \
        if (localError) \
        { \
            LWIO_LOG_ERROR("Failed to destroy semaphore [code: %d]", localError); \
        } \
    } while (0)
#endif /* __LWI_DARWIN__ */

#endif /* __LWIO_SEMAPHORE_H__ */
