#include <bail.h>

#include <openssl/err.h>

#include <lwerror.h>

DWORD
LwSSLErrorToLwError(unsigned long ssl_error)
{
    switch (ERR_GET_REASON(ssl_error))
    {
        case ERR_R_MALLOC_FAILURE:
            return LW_ERROR_OUT_OF_MEMORY;
            break;

        case ERR_R_PASSED_NULL_PARAMETER:
            return LW_ERROR_INVALID_PARAMETER;
            break;

        case ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED:
        case ERR_R_INTERNAL_ERROR:
            return LW_ERROR_INTERNAL;
            break;

        default:
            return LW_ERROR_INTERNAL; /* XXX Add LW_ERROR_SSL_ERROR */
            break;
    }
}
