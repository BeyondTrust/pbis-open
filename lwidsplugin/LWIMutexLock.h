/*
 *  LWIMutexLock.h
 *  LWIDSPlugIn
 *
 *  Created by System Administrator on 5/31/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#ifndef __LWIMUTEXLOCK_H__
#define __LWIMUTEXLOCK_H__

#include "LWIPlugIn.h"

class LWIMutexLock
{
public:
    LWIMutexLock()
    {
        pthread_mutex_init(&_lock, NULL);
    }

    ~LWIMutexLock()
    {
    }

protected:
    LWIMutexLock(const LWIMutexLock& other);
    LWIMutexLock& operator=(const LWIMutexLock& other);

public:

    inline void acquire()
    {
        pthread_mutex_lock(&_lock);
    }

    inline void release()
    {
        pthread_mutex_unlock(&_lock);
    }

private:

    pthread_mutex_t _lock;
};

#endif /* __LWIMUTEXLOCK_H__ */
