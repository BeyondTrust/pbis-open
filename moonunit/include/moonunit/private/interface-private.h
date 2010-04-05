/*
 * Copyright (c) 2008, Brian Koropoff
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Moonunit project nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRIAN KOROPOFF ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL BRIAN KOROPOFF BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __MU_INTERFACE_PRIVATE_H__
#define __MU_INTERFACE_PRIVATE_H__

#include <moonunit/internal/boilerplate.h>
#include <moonunit/test.h>

C_BEGIN_DECLS

typedef enum MuInterfaceMeta
{
    MU_META_EXPECT,
    MU_META_TIMEOUT,
    MU_META_ITERATIONS
} MuInterfaceMeta;

typedef struct MuInterfaceToken
{
    /* Basic operations */
    void (*result)(struct MuInterfaceToken*, const MuTestResult*);
    void (*event)(struct MuInterfaceToken*, const MuLogEvent* event);
    /* Extensible meta-data channel */
    void (*meta)(struct MuInterfaceToken*, MuInterfaceMeta type, ...);
    /* Reserved */
    void* reserved1;
    void* reserved2;
    MuTest* test;
} MuInterfaceToken;

MuInterfaceToken* Mu_Interface_CurrentToken(void);
void Mu_Interface_SetCurrentTokenCallback(MuInterfaceToken* (*cb) (void* data), void* data);

C_END_DECLS

#endif
