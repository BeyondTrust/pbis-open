#ifndef __FSERV_PROTOCOL_SERVER_H__
#define __FSERV_PROTOCOL_SERVER_H__

#include "protocol.h"

/* Only the server side should know the contents of a file handle */
struct FileHandle
{
    int fd;
    OpenMode mode;
};

#endif
