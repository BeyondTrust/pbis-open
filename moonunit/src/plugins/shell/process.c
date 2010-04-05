/*
 * Copyright (c) Brian Koropoff
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

#include <config.h>

#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "process.h"

typedef int fdpair[2];

int Process_Open(Process* handle, char * const argv[],
                 unsigned long num_channels, ...)
{
    fdpair* pipes;
    int i;
    va_list ap;
    pid_t pid;
    int status = 0;
    
    /* Set up channels */
    
    handle->num_channels = num_channels;
    handle->channels = calloc(num_channels, sizeof(ProcessChannel));
    pipes = calloc(num_channels * 2, sizeof(int));
    
    va_start (ap, num_channels);

    for (i = 0; i < num_channels; i++)
    {
        ProcessChannelDirection dir = va_arg(ap, ProcessChannelDirection);

        switch (dir)
        {
        case PROCESS_CHANNEL_IN:
            if (pipe(pipes[i]))
            {
                status = -1;
                goto error;
            }
            handle->channels[i].fd = pipes[i][0];
            break;
        case PROCESS_CHANNEL_OUT:
            if (pipe(pipes[i]))
            {
                status = -1;
                goto error;
            }
            handle->channels[i].fd = pipes[i][1];
            break;
        case PROCESS_CHANNEL_NULL_IN:
            handle->channels[i].fd = -1;
            pipes[i][1] = open("/dev/null", O_RDONLY);
            break;
        case PROCESS_CHANNEL_NULL_OUT:
            handle->channels[i].fd = -1;
            pipes[i][0] = open("/dev/null", O_WRONLY);
            break;
        case PROCESS_CHANNEL_DEFAULT:
            handle->channels[i].fd = -1;
        }

        handle->channels[i].direction = dir;
    }

    va_end(ap);

    /* Create child process */
    
    if ((pid = fork()) == 0)
    {
        /* Child */
        
        /* Become the leader of our own process group
         * so that our children will be terminated when
         * we are.
         */
#if defined(HAVE_SETPGID)
        setpgid(0, getpid());
#elif defined(HAVE_SETPGRP)
        setpgrp();
#endif

        for (i = 0; i < num_channels; i++)
        {
            switch (handle->channels[i].direction)
            {
            case PROCESS_CHANNEL_IN:
            case PROCESS_CHANNEL_NULL_IN:
                if (pipes[i][0] >= 0)
                    close(pipes[i][0]);
                dup2(pipes[i][1], i);
                break;
            case PROCESS_CHANNEL_OUT:
            case PROCESS_CHANNEL_NULL_OUT:
                if (pipes[i][1] >= 0)
                    close(pipes[i][1]);
                dup2(pipes[i][0], i);
                break;
            default:
                break;
            }
        }

        execvp(*argv, argv);
    }
    else
    {
        handle->pid = pid;
        for (i = 0; i < num_channels; i++)
        {
            switch (handle->channels[i].direction)
            {
            case PROCESS_CHANNEL_IN:
                if (pipes[i][1] >= 0)
                    close(pipes[i][1]);
                break;
            case PROCESS_CHANNEL_OUT:
                if (pipes[i][0] >= 0)
                    close(pipes[i][0]);
                break;
            default:
                break;
            }
        }
    }

error:

    if (pipes)
        free(pipes);

    return status;
}

int
Process_Channel_ReadLine(Process* handle, unsigned int cnum, char** out)
{
    int res = 0;
    ProcessChannel* channel = &handle->channels[cnum];
    unsigned int filled;
    char* newline;

    if (!channel->buffer)
    {
        channel->bufferlen = 1024;
        channel->buffer = malloc(channel->bufferlen);
        channel->buffer[0] = '\0';
    }

    newline = strchr(channel->buffer, '\n');

    while (!newline)
    {
        filled = strlen(channel->buffer);
    
        if (filled + 1 == channel->bufferlen)
        {
            channel->bufferlen *= 2;
            channel->buffer = realloc(channel->buffer, channel->bufferlen);
        }

        res = read(channel->fd, channel->buffer + filled, channel->bufferlen - filled - 1);

        if (res <= 0)
        {
            newline = NULL;
            break;
        }
        else
        {
            channel->buffer[res] = '\0';
            newline = strchr(channel->buffer, '\n');
        }
    }

    if (newline)
    {
        res = newline - channel->buffer + 1;
        *out = malloc(res + 1);
        memcpy(*out, channel->buffer, res);
        (*out)[res] = '\0';

        memmove(channel->buffer, channel->buffer + res, channel->bufferlen - res);
        newline = strchr(channel->buffer, '\n');
    }
    
    channel->ready = (newline != NULL);

    return res;
}

int
Process_Channel_Write(Process* handle, unsigned int cnum, const void* data, size_t len)
{
    ProcessChannel* channel = &handle->channels[cnum];
    
    return write(channel->fd, data, len);
}

static
void
timeval_diff(struct timeval* from, struct timeval* to, struct timeval* diff)
{
    diff->tv_sec = to->tv_sec - from->tv_sec;
    diff->tv_usec = to->tv_usec - from->tv_usec;

    while (diff->tv_usec < 0)
    {
        diff->tv_usec += 1000000;
        diff->tv_sec -= 1;
    }

    while (diff->tv_usec > 1000000)
    {
        diff->tv_sec += 1;
        diff->tv_usec -= 1000000;
    }
}

int
Process_Select(Process* handle, ProcessTimeout* abs, unsigned int cnum, ...)
{
    int maxfd = 0;
    fd_set readfds;
    fd_set writefds;
    int i;
    int res;
    int already_ready = 0;
    struct timeval timeout, now, target;
    va_list ap;

    FD_ZERO(&readfds);
    FD_ZERO(&writefds);

    va_start(ap, cnum);

    for (i = 0; i < cnum; i++)
    {
        unsigned int cindex = va_arg(ap, unsigned int);
        ProcessChannel* channel = &handle->channels[cindex];
        
        if (channel->ready)
            already_ready++;

        if (channel->fd > maxfd)
            maxfd = channel->fd;
        
        switch (channel->direction)
        {
        case PROCESS_CHANNEL_IN:
            FD_SET(channel->fd, &readfds);
            break;
        case PROCESS_CHANNEL_OUT:
            FD_SET(channel->fd, &writefds);
            break;
        default:
            break;
        }
    }

    va_end(ap);

    if (already_ready)
        return already_ready;

    if (abs)
    {
        target.tv_sec = abs->seconds;
        target.tv_usec = abs->microseconds;
        
        gettimeofday(&now, NULL);
        
        timeval_diff(&now, &target, &timeout);
    }

    res = select(maxfd + 1, &readfds, &writefds, NULL, abs ? &timeout : NULL);
    
    if (res >= 1)
    {
        for (i = 0; i < handle->num_channels; i++)
        {
            ProcessChannel* channel = &handle->channels[i];

            switch (channel->direction)
            {
            case PROCESS_CHANNEL_IN:
                if (FD_ISSET(channel->fd, &readfds))
                    channel->ready = 1;
                break;
            case PROCESS_CHANNEL_OUT:
                if (FD_ISSET(channel->fd, &writefds))
                    channel->ready = 1;
                break;
            default:
                break;
            }
        }
    }

    return res;
}

int
Process_Finished(Process* handle, int* status)
{
    return waitpid(handle->pid, status, WNOHANG) == handle->pid;
}

int
Process_Channel_Ready(Process* handle, int cnum)
{
    return handle->channels[cnum].ready;
}

void
Process_Close(Process* handle)
{
    int i;

    for (i = 0; i < handle->num_channels; i++)
    {
        if (handle->channels[i].fd >= 0)
            close (handle->channels[i].fd);
        if (handle->channels[i].buffer)
            free(handle->channels[i].buffer);
    }

    free(handle->channels);

    kill(-handle->pid, SIGTERM);
    waitpid(handle->pid, NULL, 0);
}

void
Process_GetTime(ProcessTimeout* timeout, unsigned long msoffset)
{
    struct timeval timeval;

    gettimeofday(&timeval, NULL);

    timeout->seconds = timeval.tv_sec;
    timeout->microseconds = timeval.tv_usec + msoffset * 1000;
    
    while (timeout->microseconds > 1000000)
    {
        timeout->seconds++;
        timeout->microseconds -= 1000000;
    }
}
