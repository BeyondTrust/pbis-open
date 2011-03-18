#ifndef __LWIO_TOOL_H__

#include "config.h"
#include "lwiosys.h"
#include <lwio/ntfileapi.h>
#include "lwiodef.h"
#include "lwioutils.h"
#include "lwparseargs.h"
#include <lw/rtlgoto.h>
#include "ntlogmacros.h"

NTSTATUS
IoTestMain(
    IN OUT PLW_PARSE_ARGS pParseArgs,
    OUT PSTR* ppszUsageError
    );

NTSTATUS
SrvTestMain(
    IN OUT PLW_PARSE_ARGS pParseArgs,
    OUT PSTR* ppszUsageError
    );

#endif /* __LWIO_TOOL_H__ */
