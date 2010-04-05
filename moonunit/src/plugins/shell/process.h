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

#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <unistd.h>
#include <stdio.h>

#include <moonunit/private/util.h>

typedef enum
{
    PROCESS_CHANNEL_OUT,
    PROCESS_CHANNEL_IN,
    PROCESS_CHANNEL_DEFAULT,
    PROCESS_CHANNEL_NULL_IN,
    PROCESS_CHANNEL_NULL_OUT
} ProcessChannelDirection;

typedef struct
{
    ProcessChannelDirection direction;
    int fd;
    int ready;
    char* buffer;
    size_t bufferlen;
} ProcessChannel;

typedef struct
{
    pid_t pid;
    ProcessChannel* channels;
    unsigned long num_channels;
} Process;

typedef struct
{
    long seconds;
    long microseconds;
} ProcessTimeout;

int Process_Open(Process* handle, char * const argv[],
                 unsigned long num_channels, ...);
int Process_Channel_ReadLine(Process* handle, unsigned int channel, char** out);
int Process_Channel_Write(Process* handle, unsigned int channel, const void* data, size_t len);
int Process_Select(Process* handle, ProcessTimeout* abs, unsigned int cnum, ...);
int Process_Channel_Ready(Process* handle, int cnum);
int Process_Finished(Process* handle, int* status);
void Process_Close(Process* handle);
void Process_GetTime(ProcessTimeout* timeout, unsigned long msoffset);

#endif
