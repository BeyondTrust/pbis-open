#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <dce/rpc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

    rpc_binding_from_string_binding(
        (unsigned_char_t*) argv[1],
        &binding,
        &status);

    if (status)
    {
        goto error;
    }

    rpc_mgmt_inq_stats(
        binding,
        &stats,
        &status);

    if (status)
    {
        goto error;
    }

    printf("Statistics: %u\n", stats->count);
    for (i = 0; i < stats->count; i++)
    {
        printf("  [%u] = %u\n", i, stats->stats[i]);
    }

cleanup:

    return status ? 1 : 0;

error:

    fprintf(stderr, "Error: %x\n", status);

    goto cleanup;
}
