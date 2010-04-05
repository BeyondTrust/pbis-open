#include "protocol.h"

#include <lwmsg/lwmsg.h>

/* Begin type specifications */

/* FileHandle (opaque) */
LWMsgTypeSpec filehandle_spec[] =
{
    /* Identify type name of handle */
    LWMSG_HANDLE(FileHandle),
    /* End specification */
    LWMSG_TYPE_END
};

/* OpenRequest */
LWMsgTypeSpec openrequest_spec[] =
{
    /* Begin structure */
    LWMSG_STRUCT_BEGIN(OpenRequest),
    /* path - marshal as pointer to string */
    LWMSG_MEMBER_PSTR(OpenRequest, path),
    /* mode - marshal as 8-bit unsigned integer */
    LWMSG_MEMBER_UINT8(OpenRequest, mode),
    /* End structure */
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/* WriteRequest */
LWMsgTypeSpec writerequest_spec[] =
{
    /* Begin structure */
    LWMSG_STRUCT_BEGIN(WriteRequest),
    /* handle - marshal as FileHandle (references existing spec) */
    LWMSG_MEMBER_TYPESPEC(WriteRequest, handle, filehandle_spec),
    /* size - marshal as 64-bit unsigned integer */
    LWMSG_MEMBER_UINT64(WriteRequest, size),
    /* data - marshal as pointer */
    LWMSG_MEMBER_POINTER_BEGIN(WriteRequest, data),
    /* marshall pointees as 8-bit unsigned integers (in-memory type is char) */
    LWMSG_UINT8(char),
    /* End pointer */
    LWMSG_POINTER_END,
    /* Length of data is equal to value of size member */
    LWMSG_ATTR_LENGTH_MEMBER(WriteRequest, size),
    /* End structure */
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/* ReadRequest */
LWMsgTypeSpec readrequest_spec[] =
{
    /* Begin structure */
    LWMSG_STRUCT_BEGIN(ReadRequest),
    /* handle - marshal as FileHandle (references existing spec) */
    LWMSG_MEMBER_TYPESPEC(ReadRequest, handle, filehandle_spec),
    /* size - marshal as 64-bit unsigned integer */
    LWMSG_MEMBER_UINT64(ReadRequest, size),
    /* End structure */
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/* StatusReply */
LWMsgTypeSpec statusreply_spec[] =
{
    /* Begin structure */
    LWMSG_STRUCT_BEGIN(StatusReply),
    /* err - marshal as 32-bit unsigned integer */
    LWMSG_MEMBER_UINT32(StatusReply, err),
    /* End structure */
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/* ReadReply */
LWMsgTypeSpec readreply_spec[] =
{
    /* Begin structure */
    LWMSG_STRUCT_BEGIN(ReadReply),
    /* size - marshal as 64-bit unsigned integer */
    LWMSG_MEMBER_UINT64(ReadReply, size),
    /* data - marshal as pointer */
    LWMSG_MEMBER_POINTER_BEGIN(ReadReply, data),
    /* marshall pointees as 8-bit unsigned integers (in-memory type is char) */
    LWMSG_UINT8(char),
    /* End pointer */
    LWMSG_POINTER_END,
    /* Length of data is equal to value of size member */
    LWMSG_ATTR_LENGTH_MEMBER(ReadReply, size),
    /* End structure */
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/* End type specifications */

/* Begin protocol specification */
LWMsgProtocolSpec protocol_spec[] =
{
    LWMSG_MESSAGE(FSERV_OPEN_REQ, openrequest_spec),
    LWMSG_MESSAGE(FSERV_OPEN_RES, filehandle_spec),
    LWMSG_MESSAGE(FSERV_READ_REQ, readrequest_spec),
    LWMSG_MESSAGE(FSERV_READ_RES, readreply_spec),
    LWMSG_MESSAGE(FSERV_WRITE_REQ, writerequest_spec),
    LWMSG_MESSAGE(FSERV_CLOSE_REQ, filehandle_spec),
    LWMSG_MESSAGE(FSERV_VOID_RES, NULL),
    LWMSG_MESSAGE(FSERV_ERROR_RES, statusreply_spec),
    LWMSG_PROTOCOL_END
};
/* End protocol specification */

LWMsgProtocolSpec*
fserv_get_protocol(void)
{
    return protocol_spec;
}
