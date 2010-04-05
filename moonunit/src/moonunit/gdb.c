/*
 * Copyright (c) 2007, Brian Koropoff
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

#include "gdb.h"

#include <config.h>
#include <moonunit/private/util.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#ifdef HAVE_SYS_IOCTL_H
#    include <sys/ioctl.h>
#endif
#include <errno.h>
#include <string.h>

static int
my_tcsetpgrp(int fd, pid_t pid)
{
#if defined(HAVE_TCSETPGRP)
    return tcsetpgrp(fd, pid);
#elif defined(TIOCSPGRP)
    return ioctl(fd, TIOCSPGRP, &pid);
#else
#    error No method to set foreground process group available
#endif
}


static pid_t
my_tcgetpgrp(int fd)
{
#if defined(HAVE_TCGETPGRP)
    return tcgetpgrp(fd);
#elif defined(TIOCGPGRP)
    pid_t pgrp;
    ioctrl(fd, TIOCGPGRP, &pgrp);
    return pgrp;
#else
#   error No method to get foreground process group available
#endif
}

void gdb_attach_interactive(const char* program, pid_t pid, const char* breakpoint)
{
    char template[] = "/tmp/mu_gdbinit_XXXXXX";
    int fd = mkstemp(template);
    pid_t child;

    if (fd < 0)
        return;

    FILE* file = fdopen(fd, "w");

    fprintf(file, "handle SIGCONT nostop noprint\n");
    fprintf(file, "break %s\n", breakpoint);
    fprintf(file, "signal SIGCONT");

    fclose(file);

    if (!(child = fork()))
    {
        pid_t self = getpid();
        while (getpgid(self) != self);

        if (execlp("gdb", "gdb",
                   (const char*) "-q",
                   (const char*) "-x",
                   (const char*) template,
                   (const char*) program, 
                   (const char*) format("%lu", pid), 
                   (const char*) NULL))
        {
            fprintf(stderr, "Could not start gdb: %s\n", strerror(errno));
            _exit(1);
        }
    }
    else
    {
        int status;
        int tty = open("/dev/tty", O_RDWR | O_NOCTTY);
        pid_t oldpgrp = my_tcgetpgrp(tty);

        /* Put child into it's own process group */
        setpgid(child, 0);
        /* Make child process group the foregroup group */
        my_tcsetpgrp(tty, child);

        /* Wait for gdb to finish */
        if (waitpid(child, &status, 0) != child || WEXITSTATUS(status))
            fprintf(stderr, "WARNING: gdb session terminated unexpectedly");

        /* Block SIGTTOU signal which can occur when resetting foreground group */
        signal(SIGTTOU, SIG_IGN);
        /* Restore old foregroup process group */
        my_tcsetpgrp(tty, oldpgrp);
        /* Reset SIGTTOU back to default */
        signal(SIGTTOU, SIG_DFL);
        close(tty);
    }

    unlink(template);
}

void gdb_attach_backtrace(const char* program, pid_t pid, char **backtrace)
{
    char* buffer;
    unsigned int capacity = 2048;
    char *command;
    char template[] = "/tmp/mu_gdbinit_XXXXXX";
    unsigned int position;
    size_t bytes;
    FILE* file;

    int fd = mkstemp(template);

    if (fd < 0)
        return;

    file = fdopen(fd, "w");   

    fprintf(file, "bt");

    fclose(file);

    command = format("gdb '%s' %lu -x '%s' --batch'", program, (unsigned long) pid, template);

    file = popen(command, "r");

    if (!file)
        return;

    buffer = malloc(capacity * sizeof(*buffer));
    position = 0;

    while ((bytes = fread(buffer + position, capacity - position - 1, sizeof(*buffer), file)) > 0)
    {
        position += bytes;
        if (position >= capacity - 1)
        {
            capacity *= 2;
            buffer = realloc(buffer, capacity * sizeof(*buffer));
        }
    }

    pclose(file);

    free(command);
    unlink(template);

    *backtrace = buffer;
}
