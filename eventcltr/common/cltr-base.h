/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Core Eventlog header
 *
 */

#ifdef _WIN32
#include <windows.h>
#endif

#include <stdio.h>

#ifdef _WIN32
#include <rpc.h>
#else
#include <lwrpcrt.h>
#endif

#include <wc16str.h>
#include <wc16printf.h>
#include <ctype.h>

#define SECURITY_WIN32
#ifdef _WIN32
#include <security.h>
#include <ntsecapi.h>
#endif

#include <time.h> //added for evtparser
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>

//#include <direct.h> 
#include <wchar.h> //required for fileutils
#define LW_STRICT_NAMESPACE
#include <lw/rtlstring.h>

#include "cltr-struct.h"
#include "eventapi.h"
#include "cltr-error.h"
#include "cltr-logging.h"
#include "cltr-mem.h"
#include "cltr-rpcmem.h"
#include "cltr-str.h"
#include "cltr-stack.h"
#include "cltr-cfg.h"
#include "cltr-futils.h"
#include <errno.h>


#ifdef _WIN32

#include <Rpc.h>
#pragma warning(disable : 4996)
#else

#include "config.h"
#include "cltr-futils.h"

#endif

#ifndef HAVE_VSYSLOG
#define vsyslog sys_vsyslog
#endif

