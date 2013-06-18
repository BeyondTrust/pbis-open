#ifndef DCERPC_H
#define DCERPC_H
#if !defined(USE_DCE_STYLE) && !defined(USE_MS_STYLE)
#error Define either USE_DCE_STYLE or USE_MS_STYLE to decide which rpc call style should be used.
#endif

#ifndef _WIN32
#ifdef USE_DCE_STYLE
#include <dce/rpc.h>
#include <dce/pthread_exc.h>
#include <dce/dce_error.h>
#else
#include "mswrappers.h"
#endif
#else //_WIN32 defined
#include <rpc.h>
#ifdef USE_DCE_STYLE
#include "dce2msrpc.h"
#endif
#endif //_WIN32

#include "rpcfields.h"
#endif //DCERPC_H
