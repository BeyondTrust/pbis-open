#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "util_log.h"

void
Log(
    const char* message
    )
{
    int fd = -1;

    if (message == NULL)
        return;

    fd = open("/var/log/pbis-samba-interop.log", O_WRONLY | O_APPEND);
    if (fd != -1)
    {
        write(fd, message, strlen(message));
        close(fd);
    }
}
