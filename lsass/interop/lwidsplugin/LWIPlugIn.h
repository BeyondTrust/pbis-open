/*
 *  LWIPlugIn.h
 *  LWIDSPlugIn
 *
 *  Created by Sriram Nambakam on 5/23/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#ifndef __LWIPLUGIN_H__
#define __LWIPLUGIN_H__

// System headers
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <syslog.h>
#include <stdarg.h>
#include <string.h>
#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>

// For passwd group structures
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

#include <CoreFoundation/CoreFoundation.h>

#include <DirectoryService/DirServices.h>
#include <DirectoryService/DirServicesUtils.h>
#include <DirectoryService/DirServicesConst.h>

// Project headers

#include "PlugInShell.h"
#include "ServerModuleLib.h"
#include "PluginData.h"

#define IN
#define OUT
#define OPTIONAL

#include "LWIStruct.h"
#include "Utilities.h"
#include "LWIException.h"
#include "LWIBitVector.h"
#include "LWIAttrLookup.h"
#include "LWIRecTypeLookup.h"
#include "LWIPlugInInitException.h"
#include "LWAuthAdapter.h"
#include "LWIQuery.h"

#define PLUGIN_ROOT_PATH "/Likewise - Active Directory"

#endif /* __LWIPLUGIN_H__ */

