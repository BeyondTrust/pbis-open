#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <dce/rpc.h>
#include <dce/http.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

struct
{
    unsigned use_tls:1;
    unsigned verify_peer:1;
    unsigned verify_name:1;
    char* cert;
    char* cert_type;
    char* ca_file;
    char* server;
    unsigned int loop;
    unsigned int sleep;
} global =
{
    .use_tls = FALSE,
    .verify_peer = TRUE,
    .verify_name = TRUE,
    .cert = NULL,
    .cert_type = NULL,
    .ca_file = NULL,
    .server = NULL,
    .loop = 1,
    .sleep = 0
};

static
unsigned32
parse_args(
    int argc,
    char *argv[]
    )
{
    int i = 0;

    for (i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "--tls"))
        {
            global.use_tls = TRUE;
        }
        else if (!strcmp(argv[i], "--no-verify-peer"))
        {
            global.verify_peer = FALSE;
        }
        else if (!strcmp(argv[i], "--no-verify-name"))
        {
            global.verify_name = FALSE;
        }
        else if (!strcmp(argv[i], "--cert"))
        {
            global.cert = argv[++i];
        }
        else if (!strcmp(argv[i], "--cert-type"))
        {
            global.cert_type = argv[++i];
        }
        else if (!strcmp(argv[i], "--ca-file"))
        {
            global.ca_file = argv[++i];
        }
        else if (!strcmp(argv[i], "--loop"))
        {
            global.loop = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "--sleep"))
        {
            global.sleep = atoi(argv[++i]);
        }
        else
        {
            global.server = argv[i];
            break;
        }
    }

    return 0;
}

int
main(
    int argc,
    char *argv[]
    )
{
    rpc_binding_handle_t binding = NULL;
    rpc_stats_vector_t* stats = NULL;
    unsigned32 status = 0;
    unsigned32 i = 0;
    unsigned32 j = 0;
    rpc_transport_info_handle_t info = NULL;

    status = parse_args(argc, argv);
    if (status)
    {
        goto error;
    }

    rpc_http_transport_info_create(
        &info,
        &status);
    if (status)
    {
        goto error;
    }

    rpc_http_transport_info_set_use_tls(info, global.use_tls);
    rpc_http_transport_info_set_tls_verify_peer(info, global.verify_peer);
    rpc_http_transport_info_set_tls_verify_name(info, global.verify_name);
    if (global.cert)
    {
        rpc_http_transport_info_set_tls_cert(info, global.cert);
    }
    if (global.cert_type)
    {
        rpc_http_transport_info_set_tls_cert_type(info, global.cert_type);
    }
    if (global.ca_file)
    {
        rpc_http_transport_info_set_tls_ca_file(info, global.ca_file);
    }

    rpc_binding_from_string_binding(
        (unsigned_char_t*) global.server,
        &binding,
        &status);

    if (status)
    {
        goto error;
    }

    rpc_binding_set_transport_info(
        binding,
        info,
        &status);

    if (status)
    {
        goto error;
    }

    for (i = 0; i < global.loop; i++)
    {
        rpc_mgmt_inq_stats(
            binding,
            &stats,
            &status);
        
        if (status)
        {
            goto error;
        }
        
        printf("Statistics: %u\n", stats->count);
        for (j = 0; j < stats->count; j++)
        {
            printf("  [%u] = %u\n", j, stats->stats[j]);
        }

        sleep(global.sleep);
    }

cleanup:

    return status ? 1 : 0;

error:

    fprintf(stderr, "Error: %x\n", status);

    goto cleanup;
}
