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
    unsigned http_tls:1;
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
    .http_tls = FALSE,
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
void
usage(
    void
    )
{
    printf(
        "rpcping - ping DCE/RPC endpoint mapper\n"
        "Usage: rpcping [ options ... ] binding\n");
}

static
void
help(
    void
    )
{
    usage();

    printf(
        "\n"
        "Options:\n"
        "    --help                     Show this help\n"
        "    --loop count               Loop count times (default: 1)\n"
        "    --sleep t                  Sleep for t seconds after each ping (default: 0)\n\n"
        "Options (http)\n"
        "    --http-tls                 Use TLS with ncacn_http bindings\n"
        "    --no-verify-peer           Disable verifying server TLS certificate\n"
        "    --no-verity-name           Disable verifying server hostname against TLS certificate\n"
        "    --cert                     Specify client TLS certificate file\n"
        "    --cert-type                Specify client TLS certificate file type (PEM or DER)\n"
        "    --ca-file                  Specify CA certificate file for verifying server TLS certificate\n");
}

static
unsigned32
parse_args(
    int argc,
    char *argv[]
    )
{
    int i = 0;

    if (argc < 2)
    {
        usage();
        exit(1);
    }

    for (i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "--http-tls"))
        {
            global.http_tls = TRUE;
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
        else if (!strcmp(argv[i], "--help"))
        {
            help();
            exit(0);
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

    rpc_binding_from_string_binding(
        (unsigned_char_t*) global.server,
        &binding,
        &status);

    if (status)
    {
        goto error;
    }

    if (global.http_tls)
    {
        rpc_http_transport_info_create(
            &info,
            &status);
        if (status)
        {
            goto error;
        }
        
        rpc_http_transport_info_set_use_tls(info, global.http_tls);
        rpc_http_transport_info_set_tls_verify_peer(info, global.verify_peer);
        rpc_http_transport_info_set_tls_verify_name(info, global.verify_name);
        if (global.cert)
        {
            rpc_http_transport_info_set_tls_cert(info, (idl_char*) global.cert);
        }
        if (global.cert_type)
        {
            rpc_http_transport_info_set_tls_cert_type(info, (idl_char*) global.cert_type);
        }
        if (global.ca_file)
        {
            rpc_http_transport_info_set_tls_ca_file(info, (idl_char*) global.ca_file);
        }

        rpc_binding_set_transport_info(
            binding,
            info,
            &status);
        
        if (status)
        {
            goto error;
        }
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
        
        printf("Statistics [%u]: %u\n", (unsigned int)i, (unsigned int)stats->count);
        for (j = 0; j < stats->count; j++)
        {
            printf("  [%u] = %u\n", (unsigned int)j, (unsigned int)stats->stats[j]);
        }

        sleep(global.sleep);
    }

cleanup:

    return status ? 1 : 0;

error:

    fprintf(stderr, "Error: %x\n", (unsigned int)status);

    goto cleanup;
}
