#ifndef __FSERV_PROTOCOL_H__
#define __FSERV_PROTOCOL_H__

#include <lwmsg/lwmsg.h>

/* Begin message structures */

/* Opaque type -- actual definition in protocol-server.h */
typedef struct FileHandle FileHandle;

typedef enum OpenMode
{
    OPEN_MODE_READ = 1,
    OPEN_MODE_WRITE = 2,
    OPEN_MODE_APPEND = 4
} OpenMode;

/* Parameters sent in an open request */
typedef struct OpenRequest
{
    char* path;
    OpenMode mode;
} OpenRequest;

/* Parameters sent in a write request */
typedef struct WriteRequest
{
    LWMsgHandle* handle;
    unsigned long size;
    char* data;
} WriteRequest;

/* Parameters sent in a read request */
typedef struct ReadRequest
{
    LWMsgHandle* handle;
    unsigned long size;
} ReadRequest;

/* Generic status code reply for faild open, failed read, write, close */
typedef struct StatusReply
{
    int err;
} StatusReply;

/* Reply to a successful read request */
typedef struct ReadReply
{
    unsigned long size;
    char* data;
} ReadReply;

/* Notably missing structures:
 *
 * OpenReply - the reply type is just a LWMsgHandle if it succeeds
 * CloseRequest - the request type is just a LWMsgHandle
 */

/* End message structures */

/* Begin message enumeration */
typedef enum MessageType
{
    FSERV_OPEN_REQ,         /* OpenRequest */
    FSERV_OPEN_RES,         /* LWMsgHandle */
    FSERV_READ_REQ,         /* ReadRequest */
    FSERV_READ_RES,         /* ReadReply */
    FSERV_WRITE_REQ,        /* WriteRequest */
    FSERV_CLOSE_REQ,        /* LWMsgHandle */
    FSERV_VOID_RES,         /* void */
    FSERV_ERROR_RES         /* StatusReply */
} MessageType;
/* End message enumeration */    

LWMsgProtocolSpec*
fserv_get_protocol(void);

#define FSERV_SOCKET_PATH "/tmp/.fserv-socket"

#endif
