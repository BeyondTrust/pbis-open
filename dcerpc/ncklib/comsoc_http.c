#include <commonp.h>
#include <com.h>
#include <comprot.h>
#include <comnaf.h>
#include <comp.h>
#include <comsoc_http.h>
#include <fcntl.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cnp.h>
#include <httpnaf.h>
#include <stddef.h>
#include <cnnet.h>
/* This is a huge hack to work around curl being stupid
   on multiarch systems */
#define __CURL_CURLRULES_H
#define CURL_ISOCPP
#define CURL_OFF_T_C(Val)  Val ## CURL_SUFFIX_CURL_OFF_T
#define CURL_OFF_TU_C(Val) Val ## CURL_SUFFIX_CURL_OFF_TU
#include <curl/curl.h>
#include <uuid/uuid.h>

#define HTTP_FRAG_SIZE 4096
#define HTTP_CONTENT_LENGTH (1024*1024*1024) /* 1 GB */
#define HTTP_ESTABLISH_OUTBOUND_CONTENT_LENGTH 76
#define HTTP_KEEPALIVE 300000
#define HTTP_RECEIVE_WINDOW_SIZE 65536
#define HTTP_PACKET_TYPE 20
#define HTTP_FLAG_PING              0x1
#define HTTP_COMMAND_VERSION        0x6
#define HTTP_COMMAND_COOKIE         0x3
#define HTTP_COMMAND_LIFETIME       0x4
#define HTTP_COMMAND_KEEPALIVE      0x5
#define HTTP_COMMAND_ASSOC          0xc
#define HTTP_COMMAND_RECEIVE_WINDOW 0x0
#define HTTP_SOCKET_LOCK_SEND(sock) (dcethread_mutex_lock_throw(&(sock)->send_lock))
#define HTTP_SOCKET_LOCK_RECV(sock) (dcethread_mutex_lock_throw(&(sock)->recv_lock))
#define HTTP_SOCKET_UNLOCK_SEND(sock) (dcethread_mutex_unlock_throw(&(sock)->send_lock))
#define HTTP_SOCKET_UNLOCK_RECV(sock) (dcethread_mutex_unlock_throw(&(sock)->recv_lock))

typedef struct rpc_http_transport_info_s
{
    unsigned use_tls:1;
    unsigned tls_verify_peer:1;
    unsigned tls_verify_name:1;
    char* tls_cert;
    char* tls_cert_type;
    char* tls_ca_file;
} rpc_http_transport_info_t, *rpc_http_transport_info_p_t;

typedef struct rpc_http_socket_s
{
    rpc_http_addr_t peeraddr;
    rpc_http_addr_t localaddr;
    uuid_t connection_cookie;
    uuid_t inbound_cookie;
    uuid_t outbound_cookie;
    uuid_t assoc_group_id;
    rpc_http_transport_info_t info;
    CURL* send;
    CURL* recv;
    unsigned char* recv_buffer;
    size_t recv_buffer_len;
    size_t recv_buffer_used;
    size_t recv_buffer_index;
    int send_error;
    int recv_error;
    dcethread_mutex send_lock;
    dcethread_mutex recv_lock;
} rpc_http_socket_t, *rpc_http_socket_p_t;

INTERNAL
int
rpc__http_map_curl_error(
    CURLcode error
    )
{
    switch (error)
    {
    case CURLE_OK:
        return 0;
    case CURLE_COULDNT_CONNECT:
        return ECONNREFUSED;
    case CURLE_OUT_OF_MEMORY:
        return ENOMEM;
    case CURLE_OPERATION_TIMEDOUT:
        return ETIMEDOUT;
    case CURLE_SSL_CONNECT_ERROR:
    case CURLE_SSL_CACERT:
    case CURLE_SSL_ISSUER_ERROR:
        return EACCES;
    case CURLE_SEND_ERROR:
    case CURLE_RECV_ERROR:
        return ECONNRESET;
    case CURLE_AGAIN:
        return EAGAIN;       
    default:
        return EIO;
    }
}

void
rpc__http_transport_info_destroy(
    rpc_http_transport_info_p_t http_info
    )
{
    if (http_info->tls_cert)
    {
        free(http_info->tls_cert);
    }

    if (http_info->tls_cert_type)
    {
        free(http_info->tls_cert_type);
    }

    if (http_info->tls_ca_file)
    {
        free(http_info->tls_ca_file);
    }

    return;
}

void
rpc_http_transport_info_free(
    rpc_transport_info_handle_t info
    )
{
    free(info);
}

void
rpc_http_transport_info_create(
    rpc_transport_info_handle_t* info,
    unsigned32 *st
    )
{
    rpc_http_transport_info_p_t http_info = NULL;

    http_info = calloc(1, sizeof(*http_info));
    
    if (!http_info)
    {
        *st = rpc_s_no_memory;
        goto error;
    }

    http_info->use_tls = FALSE;
    http_info->tls_verify_peer = TRUE;
    http_info->tls_verify_name = TRUE;

    *info = (rpc_transport_info_handle_t) http_info;

    *st = rpc_s_ok;

error:

    if (*st != rpc_s_ok && http_info)
    {
        rpc_http_transport_info_free((rpc_transport_info_handle_t) http_info);
    }

    return;
}

void
rpc_http_transport_info_set_use_tls(
    rpc_transport_info_handle_t info,
    boolean use_tls
    )
{
    rpc_http_transport_info_p_t http_info = (rpc_http_transport_info_p_t) info;

    http_info->use_tls = use_tls;
}

void
rpc_http_transport_info_set_tls_verify_peer(
    rpc_transport_info_handle_t info,
    boolean verify_peer
    )
{
    rpc_http_transport_info_p_t http_info = (rpc_http_transport_info_p_t) info;

    http_info->tls_verify_peer = verify_peer;
}

void
rpc_http_transport_info_set_tls_verify_name(
    rpc_transport_info_handle_t info,
    boolean verify_name
    )
{
    rpc_http_transport_info_p_t http_info = (rpc_http_transport_info_p_t) info;

    http_info->tls_verify_name = verify_name;
}

void rpc_http_transport_info_set_tls_cert(
    rpc_transport_info_handle_t info,
    char* path
    )
{
    rpc_http_transport_info_p_t http_info = (rpc_http_transport_info_p_t) info;

    http_info->tls_cert = path;
}

void rpc_http_transport_info_set_tls_cert_type(
    rpc_transport_info_handle_t info,
    char* type
    )
{
    rpc_http_transport_info_p_t http_info = (rpc_http_transport_info_p_t) info;

    http_info->tls_cert_type = type;
}

void rpc_http_transport_info_set_tls_ca_file(
    rpc_transport_info_handle_t info,
    char* path
    )
{
    rpc_http_transport_info_p_t http_info = (rpc_http_transport_info_p_t) info;

    http_info->tls_ca_file = path;
}

INTERNAL
boolean
strequal(const char* str1, const char* str2)
{
    return ((str1 == NULL && str2 == NULL) ||
            (str1 != NULL && str2 != NULL && !strcmp(str1, str2)));
}

boolean
rpc__http_transport_info_equal(
    rpc_transport_info_handle_t info1,
    rpc_transport_info_handle_t info2
    )
{
    rpc_http_transport_info_p_t http_info1 = (rpc_http_transport_info_p_t) info1;
    rpc_http_transport_info_p_t http_info2 = (rpc_http_transport_info_p_t) info2;

    return 
        (http_info2 == NULL &&
         http_info1->use_tls == FALSE) ||
        (http_info2 != NULL &&
         http_info1->use_tls == http_info2->use_tls &&
         strequal(http_info1->tls_cert, http_info2->tls_cert) &&
         strequal(http_info2->tls_ca_file, http_info2->tls_ca_file));
}

INTERNAL
void
rpc__http_socket_destroy(
    rpc_http_socket_p_t sock
    )
{
    if (sock)
    {
        rpc__http_transport_info_destroy(&sock->info);
        
        dcethread_mutex_destroy_throw(&sock->send_lock);
        dcethread_mutex_destroy_throw(&sock->recv_lock);

        if (sock->recv_buffer)
        {
            free(sock->recv_buffer);
        }

        if (sock->send)
        {
            curl_easy_cleanup(sock->send);
        }

        if (sock->recv)
        {
            curl_easy_cleanup(sock->recv);
        }
        
        free(sock);
    }
    
    return;
}

INTERNAL
rpc_socket_error_t
rpc__http_socket_create(
    rpc_http_socket_p_t* out
    )
{
    rpc_http_socket_p_t sock = NULL;
    int err = 0;

    sock = calloc(1, sizeof(*sock));

    if (!sock)
    {
        err = ENOMEM;
        goto error;
    }

    /* Set up reasonable default local endpoint */
    sock->localaddr.rpc_protseq_id = RPC_C_PROTSEQ_ID_NCACN_HTTP;
    memset(&sock->localaddr.sa, 0, sizeof(sock->localaddr.sa));

    dcethread_mutex_init_throw(&sock->send_lock, NULL);
    dcethread_mutex_init_throw(&sock->recv_lock, NULL);

    *out = sock;

done:
    
    return err;

error:

    if (sock)
    {
        rpc__http_socket_destroy(sock);
    }
    
    goto done;
}

INTERNAL
int
rpc__http_raw_send(
    CURL* curl,
    const void* buffer,
    size_t len
    )
{
    int serr = 0;
    fd_set writeset;
    size_t remaining = len;
    size_t sent = 0;
    CURLcode code = 0;
    long fd = 0;
    int ret = 0;

    curl_easy_getinfo(curl, CURLINFO_LASTSOCKET, &fd);

    while (remaining)
    {
        code = curl_easy_send(curl, buffer + (len - remaining), remaining, &sent);
        
        switch (code)
        {
        case CURLE_OK:
            remaining -= sent;
            break;
        case CURLE_AGAIN:
            FD_ZERO(&writeset);
            FD_SET(fd, &writeset);

            do
            {
                ret = dcethread_select(fd + 1, NULL, &writeset, NULL, NULL);
            } while (ret < 0 && errno == EINTR);

            if (ret < 0)
            {
                serr = errno;
                goto error;
            }
            break;
        default:
            serr = rpc__http_map_curl_error(code);
            goto error;
        }
    }

error:

    return serr;
}

INTERNAL
int
rpc__http_raw_recv(
    CURL* curl,
    void* buffer,
    size_t len,
    size_t* received
    )
{
    int serr = 0;
    fd_set readset;
    size_t remaining = len;
    size_t read = 0;
    CURLcode code = 0;
    long fd = 0;
    int ret = 0;

    curl_easy_getinfo(curl, CURLINFO_LASTSOCKET, &fd);

    while (remaining)
    {
        code = curl_easy_recv(curl, buffer + (len - remaining), remaining, &read);
        
        switch (code)
        {
        case CURLE_OK:
            remaining -= read;
            goto done;
        case CURLE_AGAIN:
            FD_ZERO(&readset);
            FD_SET(fd, &readset);

            do
            {
                ret = dcethread_select(fd + 1, &readset, NULL, NULL, NULL);
            } while (ret < 0 && errno == EINTR);

            if (ret < 0)
            {
                serr = errno;
                goto error;
            }
            break;
        default:
            serr = rpc__http_map_curl_error(code);
            goto error;
        }
    }

done:

    *received = len - remaining;

    return serr;

error:

    goto done;
}

INTERNAL
void
rpc__http_format_packet_header(
    rpc_cn_common_hdr_p_t hdr,
    size_t len
    )
{
    rpc__cn_pkt_format_common(
        (rpc_cn_packet_p_t) hdr,
        HTTP_PACKET_TYPE,
        RPC_C_CN_FLAGS_FIRST_FRAG | RPC_C_CN_FLAGS_LAST_FRAG,
        (unsigned32) len,
        0,  /* auth_len */
        0,  /* call_id */
        0); /* minor_ver */
}


INTERNAL
inline
size_t
rpc__http_packet_size(
    rpc_cn_common_hdr_p_t packet
    )
{
    uint16_t result = 0;
    int packet_order = ((packet->drep[0] >> 4) & 1);
    int native_order = (NDR_LOCAL_INT_REP == ndr_c_int_big_endian) ? 0 : 1;
    
    if (packet_order != native_order)
    {
        result = SWAB_16(packet->frag_len);
    }
    else
    {
        result = packet->frag_len;
    }
    
    return (size_t) result;
}

INTERNAL
void
rpc__http_format_int16(
    unsigned char* ptr,
    size_t* offset,
    unsigned16 value
    )
{
    memcpy(ptr + *offset, &value, sizeof(value));
    *offset += sizeof(value);
}

INTERNAL
void
rpc__http_format_int32(
    unsigned char* ptr,
    size_t* offset,
    unsigned32 value
    )
{
    memcpy(ptr + *offset, &value, sizeof(value));
    *offset += sizeof(value);
}

INTERNAL
void
rpc__http_format_uuid(
    unsigned char* ptr,
    size_t* offset,
    uuid_t* uuid
    )
{
    memcpy(ptr + *offset, uuid, sizeof(*uuid));
    *offset += sizeof(*uuid);
}

INTERNAL
int
rpc__http_parse_int16(
    unsigned char* ptr,
    size_t* offset,
    unsigned16* value
    )
{
    size_t off = *offset;
    rpc_cn_common_hdr_p_t hdr = (rpc_cn_common_hdr_p_t) ptr;

    if (off + 2 > rpc__http_packet_size(hdr))
    {
        return EBADMSG;
    }
    
    *offset += 2;

    if ((hdr->drep[0] >> 4) & 1)
    {
        *value = ptr[off+1] << 8 | ptr[off];
    }
    else
    {
        *value = ptr[off] << 8 | ptr[off+1];
    }

    return 0;
}

#if 0
INTERNAL
int
rpc__http_parse_int32(
    unsigned char* ptr,
    size_t* offset,
    unsigned32* value
    )
{
    size_t off = *offset;
    rpc_cn_common_hdr_p_t hdr = (rpc_cn_common_hdr_p_t) ptr;

    if (off + 4 > rpc__http_packet_size(hdr))
    {
        return EBADMSG;
    }
    
    *offset += 4;

    if ((hdr->drep[0] >> 4) & 1)
    {
        *value = ptr[off+3] << 24 | ptr[off+2] << 16 | ptr[off+1] << 8 | ptr[off];
    }
    else
    {
        *value = ptr[off] << 24 | ptr[off+1] << 16 | ptr[off+2] << 8 | ptr[off+3];
    }

    return 0;
}
#endif

INTERNAL
void
rpc__http_format_inbound_packet(
    rpc_http_socket_p_t sock,
    unsigned char* buffer,
    size_t* len
    )
{
    rpc_cn_common_hdr_p_t hdr = (rpc_cn_common_hdr_p_t) buffer;
    size_t offset = 0;

    offset += sizeof(*hdr);

    /* Write flags field */
    rpc__http_format_int16(buffer, &offset, 0);

    /* Write number of commands */
    rpc__http_format_int16(buffer, &offset, 6);

    /* Write version command */
    rpc__http_format_int32(buffer, &offset, HTTP_COMMAND_VERSION);
    rpc__http_format_int32(buffer, &offset, 1);

    /* Write connection cookie command */
    rpc__http_format_int32(buffer, &offset, HTTP_COMMAND_COOKIE);
    rpc__http_format_uuid(buffer, &offset, &sock->connection_cookie);

    /* Write inbound cookie command */
    rpc__http_format_int32(buffer, &offset, HTTP_COMMAND_COOKIE);
    rpc__http_format_uuid(buffer, &offset, &sock->inbound_cookie);

    /* Write channel lifetime command */
    rpc__http_format_int32(buffer, &offset, HTTP_COMMAND_LIFETIME);
    rpc__http_format_int32(buffer, &offset, HTTP_CONTENT_LENGTH);

    /* Write client keepalive command */
    rpc__http_format_int32(buffer, &offset, HTTP_COMMAND_KEEPALIVE);
    rpc__http_format_int32(buffer, &offset, HTTP_KEEPALIVE);

    /* Write assoc group id command */
    rpc__http_format_int32(buffer, &offset, HTTP_COMMAND_ASSOC);
    rpc__http_format_uuid(buffer, &offset, &sock->assoc_group_id);

    /* Format header */
    rpc__http_format_packet_header(hdr, offset);

    *len = offset;
}

INTERNAL
void
rpc__http_format_outbound_packet(
    rpc_http_socket_p_t sock,
    unsigned char* buffer,
    size_t* len
    )
{
    rpc_cn_common_hdr_p_t hdr = (rpc_cn_common_hdr_p_t) buffer;
    size_t offset = 0;

    offset += sizeof(*hdr);

    /* Write flags field */
    rpc__http_format_int16(buffer, &offset, 0);

    /* Write number of commands */
    rpc__http_format_int16(buffer, &offset, 4);

    /* Write version command */
    rpc__http_format_int32(buffer, &offset, HTTP_COMMAND_VERSION);
    rpc__http_format_int32(buffer, &offset, 1);

    /* Write connection cookie command */
    rpc__http_format_int32(buffer, &offset, HTTP_COMMAND_COOKIE);
    rpc__http_format_uuid(buffer, &offset, &sock->connection_cookie);

    /* Write outbound cookie command */
    rpc__http_format_int32(buffer, &offset, HTTP_COMMAND_COOKIE);
    rpc__http_format_uuid(buffer, &offset, &sock->outbound_cookie);

    /* Write receive window size */
    rpc__http_format_int32(buffer, &offset, HTTP_COMMAND_RECEIVE_WINDOW);
    rpc__http_format_int32(buffer, &offset, HTTP_RECEIVE_WINDOW_SIZE);

    /* Format header */
    rpc__http_format_packet_header(hdr, offset);

    *len = offset;
}

INTERNAL
void
rpc__http_format_ping_response_packet(
    rpc_http_socket_p_t sock,
    unsigned char* buffer,
    size_t* len
    )
{
    rpc_cn_common_hdr_p_t hdr = (rpc_cn_common_hdr_p_t) buffer;
    size_t offset = 0;

    offset += sizeof(*hdr);

    /* Write flags field */
    rpc__http_format_int16(buffer, &offset, HTTP_FLAG_PING);

    /* Write number of commands */
    rpc__http_format_int16(buffer, &offset, 0);

    /* Format header */
    rpc__http_format_packet_header(hdr, offset);

    *len = offset;
}

INTERNAL
int
rpc__http_send_rpc_in_data(
    rpc_http_socket_p_t sock
    )
{
    int serr = 0;
    char buffer[HTTP_FRAG_SIZE];
    char* rpcproxy = sock->peeraddr.rpc_proxy[0] ? sock->peeraddr.rpc_proxy : sock->peeraddr.server;

    snprintf(buffer, sizeof(buffer),
             "RPC_IN_DATA /rpc/rpcproxy.dll?%s:%i HTTP/1.1\r\n"
             "Accept: application/rpc\r\n"
             "User-Agent: Likewise RPC\r\n"
             "Host: %s\r\n"
             "Content-Length: %lu\r\n"
             "Connection: Keep-Alive\r\n"
             "Cache-Control: no-cache\r\n"
             "Pragma: no-cache\r\n"
             "\r\n",
             sock->peeraddr.server, sock->peeraddr.endpoint,
             rpcproxy,
             (unsigned long) HTTP_CONTENT_LENGTH);

    serr = rpc__http_raw_send(sock->send, buffer, strlen(buffer));
    if (serr)
    {
        goto error;
    }

error:

    return serr;
}

INTERNAL
int
rpc__http_connect_send(
    rpc_http_socket_p_t sock
    )
{
    int serr = 0;
    char url[512];
    char* rpcproxy = sock->peeraddr.rpc_proxy[0] ? sock->peeraddr.rpc_proxy : sock->peeraddr.server;
    const char* prot = sock->info.use_tls ? "https" : "http";
    CURLcode code = 0;

    snprintf(url, sizeof(url), "%s://%s/rpc/rpcproxy.dll?%s:%i",
             prot,
             rpcproxy,
             sock->peeraddr.server,
             sock->peeraddr.endpoint);

    curl_easy_setopt(sock->send, CURLOPT_URL, url);
    curl_easy_setopt(sock->send, CURLOPT_CONNECT_ONLY, 1L);
    curl_easy_setopt(sock->send, CURLOPT_SSL_VERIFYPEER, (long) sock->info.tls_verify_peer);
    curl_easy_setopt(sock->send, CURLOPT_SSL_VERIFYHOST, (long) sock->info.tls_verify_name);
    if (sock->info.tls_cert)
    {
        curl_easy_setopt(sock->send, CURLOPT_SSLCERT, sock->info.tls_cert);
    }
    if (sock->info.tls_cert_type)
    {
        curl_easy_setopt(sock->send, CURLOPT_SSLCERTTYPE, sock->info.tls_cert_type);
    }
    if (sock->info.tls_ca_file)
    {
        curl_easy_setopt(sock->send, CURLOPT_CAINFO, sock->info.tls_ca_file);
    }

    code = curl_easy_perform(sock->send);
    if (code != CURLE_OK)
    {
        serr = rpc__http_map_curl_error(code);
        goto error;
    }

    serr = rpc__http_send_rpc_in_data(sock);
    if (serr)
    {
        goto error;
    }

error:

    return serr;
}

INTERNAL
int
rpc__http_establish_inbound(
    rpc_http_socket_p_t sock
    )
{
    int serr = 0;
    union
    {
        rpc_cn_common_hdr_t hdr;
        unsigned char buffer[HTTP_FRAG_SIZE];
    } u;
    size_t len = sizeof(u);

    rpc__http_format_inbound_packet(sock, u.buffer, &len);

    serr = rpc__http_raw_send(sock->send, u.buffer, len);
    if (serr)
    {
        goto error;
    }

error:

    return serr;
}

INTERNAL
int
rpc__http_establish_outbound(
    rpc_http_socket_p_t sock
    )
{
    int serr = 0;
    union
    {
        rpc_cn_common_hdr_t hdr;
        unsigned char buffer[HTTP_FRAG_SIZE];
    } u;
    size_t len = sizeof(u);

    rpc__http_format_outbound_packet(sock, u.buffer, &len);

    serr = rpc__http_raw_send(sock->recv, u.buffer, len);
    if (serr)
    {
        goto error;
    }

error:

    return serr;
}

INTERNAL
int
rpc__http_send_ping_response(
    rpc_http_socket_p_t sock
    )
{
    int serr = 0;
    union
    {
        rpc_cn_common_hdr_t hdr;
        unsigned char buffer[HTTP_FRAG_SIZE];
    } u;
    size_t len = sizeof(u);

    rpc__http_format_ping_response_packet(sock, u.buffer, &len);

    HTTP_SOCKET_LOCK_SEND(sock);

    serr = rpc__http_raw_send(sock->send, u.buffer, len);
    if (serr)
    {
        goto error;
    }

error:

    HTTP_SOCKET_UNLOCK_SEND(sock);

    return serr;
}


INTERNAL
inline
size_t
rpc__http_recv_packet_size(
    rpc_http_socket_p_t sock
    )
{
    rpc_cn_common_hdr_p_t packet = (rpc_cn_common_hdr_p_t) sock->recv_buffer;

    if (sock->recv_buffer_used < sizeof(*packet))
    {
        return sizeof(*packet);
    }
    else
    {
        return rpc__http_packet_size(packet);
    }
}

INTERNAL
inline
int
rpc__http_recv_packet_type(
    rpc_http_socket_p_t sock
    )
{
    rpc_cn_common_hdr_p_t packet = (rpc_cn_common_hdr_p_t) sock->recv_buffer;

    return packet->ptype;
}

INTERNAL
void
rpc_http_clear_recv_packet(
    rpc_http_socket_p_t sock
    )
{
    size_t packet_size = rpc__http_recv_packet_size(sock);

    memmove(sock->recv_buffer, sock->recv_buffer + packet_size, sock->recv_buffer_used - packet_size);

    sock->recv_buffer_used -= packet_size;
}

int
rpc__http_send_rpc_out_data(
    rpc_http_socket_p_t sock
    )
{
    int serr = 0;
    char buffer[HTTP_FRAG_SIZE];
    char* rpcproxy = sock->peeraddr.rpc_proxy[0] ? sock->peeraddr.rpc_proxy : sock->peeraddr.server;

    snprintf(buffer, sizeof(buffer),
             "RPC_OUT_DATA /rpc/rpcproxy.dll?%s:%i HTTP/1.1\r\n"
             "Accept: application/rpc\r\n"
             "User-Agent: Likewise RPC\r\n"
             "Host: %s\r\n"
             "Content-Length: %lu\r\n"
             "Connection: Keep-Alive\r\n"
             "Cache-Control: no-cache\r\n"
             "Pragma: no-cache\r\n"
             "\r\n",
             sock->peeraddr.server, sock->peeraddr.endpoint,
             rpcproxy,
             (unsigned long) HTTP_ESTABLISH_OUTBOUND_CONTENT_LENGTH);

    serr = rpc__http_raw_send(sock->recv, buffer, strlen(buffer));
    if (serr)
    {
        goto error;
    }

error:

    return serr;
}

INTERNAL
int
rpc__http_connect_recv(
    rpc_http_socket_p_t sock
    )
{
    int serr = 0;
    char url[512];
    char* rpcproxy = sock->peeraddr.rpc_proxy[0] ? sock->peeraddr.rpc_proxy : sock->peeraddr.server;
    const char* prot = sock->info.use_tls ? "https" : "http";
    CURLcode code = 0;

    snprintf(url, sizeof(url), "%s://%s/rpc/rpcproxy.dll?%s:%i",
             prot,
             rpcproxy,
             sock->peeraddr.server,
             sock->peeraddr.endpoint);

    curl_easy_setopt(sock->recv, CURLOPT_URL, url);
    curl_easy_setopt(sock->recv, CURLOPT_CONNECT_ONLY, 1L);
    curl_easy_setopt(sock->recv, CURLOPT_SSL_VERIFYPEER, (long) sock->info.tls_verify_peer);
    curl_easy_setopt(sock->recv, CURLOPT_SSL_VERIFYHOST, (long) sock->info.tls_verify_name);
    if (sock->info.tls_cert)
    {
        curl_easy_setopt(sock->recv, CURLOPT_SSLCERT, sock->info.tls_cert);
    }
    if (sock->info.tls_cert_type)
    {
        curl_easy_setopt(sock->recv, CURLOPT_SSLCERTTYPE, sock->info.tls_cert_type);
    }
    if (sock->info.tls_ca_file)
    {
        curl_easy_setopt(sock->recv, CURLOPT_CAINFO, sock->info.tls_ca_file);
    }

    code = curl_easy_perform(sock->recv);
    if (code != CURLE_OK)
    {
        serr = rpc__http_map_curl_error(code);
        goto error;
    }    

    serr = rpc__http_send_rpc_out_data(sock);
    if (serr)
    {
        goto error;
    }

error:

    return serr;
}

INTERNAL
char*
rpc__http_find_header_termination(
    char* buffer,
    size_t len
    )
{
    char* cursor = NULL;
    unsigned int cr = FALSE;
    unsigned int crlf = FALSE;

    for (cursor = buffer; cursor < buffer + len; cursor++)
    {
        switch (*cursor)
        {
        case '\r':
            cr = TRUE;
            break;
        case '\n':
            if (cr)
            {
                if (crlf)
                {
                    return cursor + 1;
                }
                else
                {
                    crlf = TRUE;
                    cr = FALSE;
                }
            }
            break;
        default:
            cr = crlf = FALSE;
            break;
        }
    }

    return NULL;
}

INTERNAL
int
rpc__http_recv_http_response(
    rpc_http_socket_p_t sock
    )
{
    int serr = 0;
    static const char* expected = "HTTP/1.1 200";
    size_t len = 0;
    char* term = NULL;

    while ((term = rpc__http_find_header_termination(
                (char*) sock->recv_buffer,
                sock->recv_buffer_used)) == NULL)
    {
        if (sock->recv_buffer_len - sock->recv_buffer_used == 0)
        {
            /* Buffer overrun */
            serr = EBADMSG;
            goto error;
        }

        serr = rpc__http_raw_recv(
            sock->recv,
            sock->recv_buffer + sock->recv_buffer_used,
            sock->recv_buffer_len - sock->recv_buffer_used,
            &len);
        if (serr)
        {
            goto error;
        }

        sock->recv_buffer_used += len;
    }

    if (strncmp((char*) sock->recv_buffer, expected, strlen(expected)))
    {
        /* Did not get success */
        serr = EBADMSG;
        goto error;
    }

    len = term - (char*) sock->recv_buffer;

    /* Settle remaining data */
    memmove(sock->recv_buffer, sock->recv_buffer + len, sock->recv_buffer_used - len);
    sock->recv_buffer_used -= len;

error:

    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__http_socket_construct(
    rpc_socket_t sock,
    rpc_protseq_id_t pseq_id ATTRIBUTE_UNUSED,
    rpc_transport_info_handle_t info
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_http_socket_p_t http_sock = NULL;
    rpc_http_transport_info_p_t http_info = (rpc_http_transport_info_p_t) info;

    serr = rpc__http_socket_create(&http_sock);

    if (serr)
    {
        goto error;
    }

    if (http_info)
    {
        http_sock->info.use_tls = http_info->use_tls;
        http_sock->info.tls_verify_peer = http_info->tls_verify_peer;
        http_sock->info.tls_verify_name = http_info->tls_verify_name;
        if (http_info->tls_cert)
        {
            http_sock->info.tls_cert = strdup(http_info->tls_cert);
            if (!http_sock->info.tls_cert)
            {
                serr = ENOMEM;
                goto error;
            }
        }
        if (http_info->tls_cert_type)
        {
            http_sock->info.tls_cert_type = strdup(http_info->tls_cert_type);
            if (!http_sock->info.tls_cert_type)
            {
                serr = ENOMEM;
                goto error;
            }
        }
        if (http_info->tls_ca_file)
        {
            http_sock->info.tls_ca_file = strdup(http_info->tls_ca_file);
            if (!http_sock->info.tls_ca_file)
            {
                serr = ENOMEM;
                goto error;
            }
        }
    }

    uuid_generate_random(http_sock->connection_cookie);
    uuid_generate_random(http_sock->inbound_cookie);
    uuid_generate_random(http_sock->outbound_cookie);
    uuid_generate_random(http_sock->assoc_group_id);

    sock->data.pointer = (void*) http_sock;

done:
    
    return serr;
    
error:
    
    if (http_sock)
    {
        rpc__http_socket_destroy(http_sock);
    }
    
    goto done;
}

INTERNAL
rpc_socket_error_t
rpc__http_socket_destruct(
    rpc_socket_t sock
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;

    rpc__http_socket_destroy((rpc_http_socket_p_t) sock->data.pointer);

    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__http_socket_bind(
    rpc_socket_t sock,
    rpc_addr_p_t addr
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_http_addr_p_t httpaddr = (rpc_http_addr_p_t) addr;
    rpc_http_socket_p_t http = (rpc_http_socket_p_t) sock->data.pointer;

    http->localaddr = *httpaddr;

    return serr;
}

INTERNAL
void
rpc__http_socket_connect(
    rpc_socket_t sock,
    rpc_addr_p_t addr,
    rpc_cn_assoc_t *assoc ATTRIBUTE_UNUSED,
    unsigned32 *st
    )
{
    rpc_socket_error_t serr = 0;
    rpc_http_socket_p_t http = (rpc_http_socket_p_t) sock->data.pointer;

    memcpy(&http->peeraddr, addr, sizeof(http->peeraddr));

    http->send = curl_easy_init();
    http->recv = curl_easy_init();
    
    http->recv_buffer_len = HTTP_FRAG_SIZE;
    http->recv_buffer = malloc(http->recv_buffer_len);
    http->recv_buffer_used = 0;
    http->recv_buffer_index = 0;

    serr = rpc__http_connect_send(http);
    if (serr)
    {
        goto error;
    }

    serr = rpc__http_connect_recv(http);
    if (serr)
    {
        goto error;
    }

    serr = rpc__http_establish_inbound(http);
    if (serr)
    {
        goto error;
    }

    serr = rpc__http_establish_outbound(http);
    if (serr)
    {
        goto error;
    }

    serr = rpc__http_recv_http_response(http);
    if (serr)
    {
        goto error;
    }

error:

    if (serr)
    {
        rpc__cn_network_serr_to_status (serr, st);
    }
    else
    {
        *st = rpc_s_ok;
    }
}

INTERNAL
rpc_socket_error_t
rpc__http_socket_accept(
    rpc_socket_t sock,
    rpc_addr_p_t addr,
    rpc_socket_t *newsock
    )
{
    rpc_socket_error_t serr = ENOTSUP;

    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__http_socket_listen(
    rpc_socket_t sock,
    int backlog
    )
{
    rpc_socket_error_t serr = ENOTSUP;

    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__http_socket_sendmsg(
    rpc_socket_t sock,
    rpc_socket_iovec_p_t iov,
    int iov_len,
    rpc_addr_p_t addr ATTRIBUTE_UNUSED,
    int *cc
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_http_socket_p_t http = (rpc_http_socket_p_t) sock->data.pointer;
    int i = 0;

    HTTP_SOCKET_LOCK_SEND(http);

    *cc = 0;
    for (i = 0; i < iov_len; i++)
    {
        serr = rpc__http_raw_send(http->send, iov[i].iov_base, iov[i].iov_len);
        if (serr)
        {
            goto error;
        }
        *cc += iov[i].iov_len;
    }

error:

    HTTP_SOCKET_UNLOCK_SEND(http);

    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__http_socket_recvfrom(
    rpc_socket_t sock ATTRIBUTE_UNUSED,
    byte_p_t buf ATTRIBUTE_UNUSED,
    int len ATTRIBUTE_UNUSED,
    rpc_addr_p_t from ATTRIBUTE_UNUSED,
    int *cc ATTRIBUTE_UNUSED
)
{
    rpc_socket_error_t serr = ENOTSUP;

    fprintf(stderr, "WARNING: unsupported http socket function %s\n", __FUNCTION__);

    return serr;
}

INTERNAL
int
rpc__http_handle_proxy_packet(
    rpc_http_socket_p_t http
    )
{
    int serr = -1;
    size_t offset = sizeof(rpc_cn_common_hdr_t);
    unsigned16 flags = 0;
    unsigned16 commands = 0;
    unsigned char* packet = http->recv_buffer;
    
    serr = rpc__http_parse_int16(packet, &offset, &flags);
    if (serr) goto error;

    serr = rpc__http_parse_int16(packet, &offset, &commands);
    if (serr) goto error;

    if (commands == 0)
    {
        if (flags & HTTP_FLAG_PING)
        {
            serr = rpc__http_send_ping_response(http);
            if (serr) goto error;
        }
    }
    
error:

    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__http_socket_recvmsg(
    rpc_socket_t sock,
    rpc_socket_iovec_p_t iov,
    int iov_len,
    rpc_addr_p_t addr,
    int *cc
)
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_http_socket_p_t http = (rpc_http_socket_p_t) sock->data.pointer;
    size_t len = 0;
    int i = 0;

    HTTP_SOCKET_LOCK_RECV(http);

    /* Keep reading until we get a complete, non-proxy PDU */
    while (rpc__http_recv_packet_size(http) > http->recv_buffer_used ||
           rpc__http_recv_packet_type(http) == HTTP_PACKET_TYPE)
    {
        /* If we don't have a complete PDU, read until we do */
        if (rpc__http_recv_packet_size(http) > http->recv_buffer_used)
        {
            while (rpc__http_recv_packet_size(http) > http->recv_buffer_len)
            {
                size_t newlen = http->recv_buffer_len * 2;
                unsigned char* newbuf = realloc(http->recv_buffer, newlen);
                
                if (!newbuf)
                {
                    serr = ENOMEM;
                    goto error;
                }

                http->recv_buffer_len = newlen;
                http->recv_buffer = newbuf;
            }

            serr = rpc__http_raw_recv(
                http->recv,
                http->recv_buffer + http->recv_buffer_used,
                rpc__http_recv_packet_size(http) - http->recv_buffer_used,
                &len);
            if (serr)
            {
                goto error;
            }
            
            http->recv_buffer_used += len;
        }
        /* If we have a proxy PDU, handle it */
        else if (rpc__http_recv_packet_type(http) == HTTP_PACKET_TYPE)
        {
            serr = rpc__http_handle_proxy_packet(http);
            if (serr)
            {
                goto error;
            }
            rpc_http_clear_recv_packet(http);
        }
    }

    /* Give PDU back to RPC runtime */
    len = rpc__http_recv_packet_size(http);
    *cc = 0;

    for (i = 0; i < iov_len; i++)
    {
        if (iov[i].iov_len < len - http->recv_buffer_index)
        {
            memcpy(
                iov[i].iov_base,
                http->recv_buffer + http->recv_buffer_index,
                iov[i].iov_len);
            http->recv_buffer_index += iov[i].iov_len;
            *cc += iov[i].iov_len;
        }
        else
        {
            memcpy(
                iov[i].iov_base,
                http->recv_buffer + http->recv_buffer_index,
                len - http->recv_buffer_index); 
            *cc += len - http->recv_buffer_index;
            http->recv_buffer_index = 0;
            rpc_http_clear_recv_packet(http);
            break;
        }
    }

error:

    HTTP_SOCKET_UNLOCK_RECV(http);

    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__http_socket_inq_endpoint(
    rpc_socket_t sock,
    rpc_addr_p_t addr
)
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_http_socket_p_t http = (rpc_http_socket_p_t) sock->data.pointer;

    if (addr->len == 0)
    {
        addr->len = sizeof(addr->sa);
    }

    addr->rpc_protseq_id = http->localaddr.rpc_protseq_id;
    memcpy(&addr->sa, &http->localaddr.sa, addr->len);

    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__http_socket_set_broadcast(
    rpc_socket_t sock ATTRIBUTE_UNUSED
)
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;

    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__http_socket_set_bufs(
    rpc_socket_t sock ATTRIBUTE_UNUSED,
    unsigned32 txsize ATTRIBUTE_UNUSED,
    unsigned32 rxsize ATTRIBUTE_UNUSED,
    unsigned32 *ntxsize ATTRIBUTE_UNUSED,
    unsigned32 *nrxsize ATTRIBUTE_UNUSED
)
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;

    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__http_socket_set_nbio(
    rpc_socket_t sock ATTRIBUTE_UNUSED
    )
{
    rpc_socket_error_t serr = ENOTSUP;

    fprintf(stderr, "WARNING: unsupported http socket function %s\n", __FUNCTION__);

    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__http_socket_set_close_on_exec(
    rpc_socket_t sock ATTRIBUTE_UNUSED
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;

    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__http_socket_getpeername(
    rpc_socket_t sock,
    rpc_addr_p_t addr
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_http_socket_p_t http = (rpc_http_socket_p_t) sock->data.pointer;

    memcpy(addr, &http->peeraddr, sizeof(http->peeraddr));
    
    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__http_socket_get_if_id(
    rpc_socket_t sock ATTRIBUTE_UNUSED,
    rpc_network_if_id_t *network_if_id ATTRIBUTE_UNUSED
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;

    *network_if_id = SOCK_STREAM;

    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__http_socket_set_keepalive(
    rpc_socket_t sock ATTRIBUTE_UNUSED
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    
    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__http_socket_nowriteblock_wait(
    rpc_socket_t sock ATTRIBUTE_UNUSED,
    struct timeval *tmo ATTRIBUTE_UNUSED
    )
{
    rpc_socket_error_t serr = ENOTSUP;
    
    fprintf(stderr, "WARNING: unsupported http socket function %s\n", __FUNCTION__);

    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__http_socket_set_rcvtimeo(
    rpc_socket_t sock ATTRIBUTE_UNUSED,
    struct timeval *tmo ATTRIBUTE_UNUSED
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;

    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__http_socket_getpeereid(
    rpc_socket_t sock ATTRIBUTE_UNUSED,
    uid_t *euid ATTRIBUTE_UNUSED,
    gid_t *egid ATTRIBUTE_UNUSED
    )
{
    rpc_socket_error_t serr = ENOTSUP;

    fprintf(stderr, "WARNING: unsupported http socket function %s\n", __FUNCTION__);

    return serr;
}

INTERNAL
int
rpc__http_socket_get_select_desc(
    rpc_socket_t sock
    )
{
    return -1;
}

INTERNAL
rpc_socket_error_t
rpc__http_socket_enum_ifaces(
    rpc_socket_t sock ATTRIBUTE_UNUSED,
    rpc_socket_enum_iface_fn_p_t efun ATTRIBUTE_UNUSED,
    rpc_addr_vector_p_t *rpc_addr_vec ATTRIBUTE_UNUSED,
    rpc_addr_vector_p_t *netmask_addr_vec ATTRIBUTE_UNUSED,
    rpc_addr_vector_p_t *broadcast_addr_vec ATTRIBUTE_UNUSED
    )
{
    rpc_socket_error_t serr = ENOTSUP;

    fprintf(stderr, "WARNING: unsupported http socket function %s\n", __FUNCTION__);

    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__http_socket_inq_transport_info(
    rpc_socket_t sock,
    rpc_transport_info_handle_t* info
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_http_socket_p_t http = (rpc_http_socket_p_t) sock->data.pointer;
    rpc_http_transport_info_p_t http_info = NULL;

    http_info = calloc(1, sizeof(*http_info));
    
    if (!http_info)
    {
        serr = ENOMEM;
        goto error;
    }

    *http_info = http->info;

    *info = (rpc_transport_info_handle_t) http_info;

error:

    if (serr)
    {
        *info = NULL;

        if (http_info)
        {
            rpc_http_transport_info_free((rpc_transport_info_handle_t) http_info);
        }
    }
    
    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__http_socket_transport_inq_access_token(
    rpc_transport_info_handle_t info,
    rpc_access_token_p_t* token
    )
{
    *token = NULL;

    return 0;
}

rpc_socket_vtbl_t rpc_g_http_socket_vtbl =
{
    .socket_construct = rpc__http_socket_construct,
    .socket_destruct = rpc__http_socket_destruct,
    .socket_bind = rpc__http_socket_bind,
    .socket_connect = rpc__http_socket_connect,
    .socket_accept = rpc__http_socket_accept,
    .socket_listen = rpc__http_socket_listen,
    .socket_sendmsg = rpc__http_socket_sendmsg,
    .socket_recvfrom = rpc__http_socket_recvfrom,
    .socket_recvmsg = rpc__http_socket_recvmsg,
    .socket_inq_endpoint = rpc__http_socket_inq_endpoint,
    .socket_set_broadcast = rpc__http_socket_set_broadcast,
    .socket_set_bufs = rpc__http_socket_set_bufs,
    .socket_set_nbio = rpc__http_socket_set_nbio,
    .socket_set_close_on_exec = rpc__http_socket_set_close_on_exec,
    .socket_getpeername = rpc__http_socket_getpeername,
    .socket_get_if_id = rpc__http_socket_get_if_id,
    .socket_set_keepalive = rpc__http_socket_set_keepalive,
    .socket_nowriteblock_wait = rpc__http_socket_nowriteblock_wait,
    .socket_set_rcvtimeo = rpc__http_socket_set_rcvtimeo,
    .socket_getpeereid = rpc__http_socket_getpeereid,
    .socket_get_select_desc = rpc__http_socket_get_select_desc,
    .socket_enum_ifaces = rpc__http_socket_enum_ifaces,
    .socket_inq_transport_info = rpc__http_socket_inq_transport_info,
    .transport_info_free = rpc_http_transport_info_free,
    .transport_info_equal = rpc__http_transport_info_equal,
    .transport_inq_access_token = rpc__http_socket_transport_inq_access_token
};
