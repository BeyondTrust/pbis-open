/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Eventlog Client RPC Binding
 *
 */
#ifndef __CLIENT_H__
#define __CLIENT_H__

#ifdef _WIN32
#include <windows.h>
#endif
#ifdef _WIN32
#include <rpc.h>
#else
#include <lwrpcrt.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cltr-mem.h"
#include "eventapi.h"
#include "cltr-struct.h"
#include <wc16str.h>
#define LW_STRICT_NAMESPACE
#include <lw/rtlstring.h>
#include "cltr-parser.h"
#include "cltr-str.h"
#include "cltr-logging.h"
#include "cltr-error.h"

#endif /* __EVENTLOGBINDING_H__ */

