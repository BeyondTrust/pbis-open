/*
 *  LWIMutexLockGuard.h
 *  LWIDSPlugIn
 *
 *  Created by System Administrator on 5/31/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#ifndef __LWIMUTEXLOCKGUARD_H__
#define __LWIMUTEXLOCKGUARD_H___

#include "LWIMutexLock.h"

class LWIMutexLockGuard
{
public:
    LWIMutexLockGuard(LWIMutexLock& lock)
        : _lock(lock)
    {
        _lock.acquire();
    }

    ~LWIMutexLockGuard()
    {
        _lock.release();
    }

protected:

    LWIMutexLockGuard(const LWIMutexLockGuard& other);
    LWIMutexLockGuard& operator=(const LWIMutexLockGuard& other);

private:

    LWIMutexLock& _lock;
};

#endif /* __LWIMUTEXLOCKGUARD_H__ */

