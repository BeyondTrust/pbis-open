/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwnet-plugin.h
 *
 * Abstract:
 *
 *        BeyondTrust Netlogon Plugin Interface
 * 
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 * 
 */

#ifndef __LWNET_PLUGIN_H__
#define __LWNET_PLUGIN_H__

#include <lw/types.h>
#include <lw/attrs.h>

typedef struct _LWNET_PLUGIN_SERVER_ADDRESS {
    LW_PSTR pszDnsName;
    LW_PSTR pszIpAddress;
} LWNET_PLUGIN_SERVER_ADDRESS, *PLWNET_PLUGIN_SERVER_ADDRESS;

typedef struct _LWNET_PLUGIN_INTERFACE LWNET_PLUGIN_INTERFACE, *PLWNET_PLUGIN_INTERFACE;

typedef LW_VOID (*LWNET_PLUGIN_CLEANUP_CALLBACK)(
    LW_IN LW_OUT PLWNET_PLUGIN_INTERFACE pInterface
    );

//
// Note that dwDsFlags can safely be ignored in a typical plugin.  The site
// is typically irrelevant if the plugin wants to just return a hardcoded
// list for a domain.  As far as the domain name, this is up to the plugin
// depending on the desired semantics for the preferred DC list.  The plugin
// could just ignore it, but that might cause network traffic if a list
// of DCs is returned that does not apply for a domain.
//

typedef LW_DWORD (*LWNET_PLUGIN_GET_DC_LIST_CALLBACK)(
    LW_IN PLWNET_PLUGIN_INTERFACE pInterface,
    LW_IN LW_PCSTR pszDnsDomainName,
    LW_IN LW_OPTIONAL LW_PCSTR pszSiteName,
    LW_IN LW_DWORD dwDsFlags,
    LW_OUT PLWNET_PLUGIN_SERVER_ADDRESS* ppDcArray,
    LW_OUT LW_PDWORD pdwDcCount
    );

typedef LW_VOID (*LWNET_PLUGIN_FREE_DC_LIST_CALLBACK)(
    LW_IN PLWNET_PLUGIN_INTERFACE pInterface,
    LW_IN LW_OUT PLWNET_PLUGIN_SERVER_ADDRESS pDcArray,
    LW_IN LW_DWORD dwDcCount
    );

//
// This interface supports getting a preferred DC list to use instead of using
// information from DNS.  If the DCs returned are unsuitable, the standard
// algorithm (using DNS) will be used as a fallback.
//

struct _LWNET_PLUGIN_INTERFACE {
    LWNET_PLUGIN_CLEANUP_CALLBACK Cleanup;
    LWNET_PLUGIN_GET_DC_LIST_CALLBACK GetDcList;
    LWNET_PLUGIN_FREE_DC_LIST_CALLBACK FreeDcList;
};

#define _LWNET_PLUGIN_VERSION 1

#define LWNET_PLUGIN_VERSION \
    ((DWORD) (sizeof(LWNET_PLUGIN_INTERFACE) << 16) | _LWNET_PLUGIN_VERSION)

typedef LW_DWORD (*LWNET_PLUGIN_GET_INTERFACE_CALLBACK)(
    LW_IN LW_DWORD dwVersion,
    LW_OUT PLWNET_PLUGIN_INTERFACE* ppInterface
    );

#define LWNET_PLUGIN_GET_INTERFACE_FUNCTION_NAME "LWNetPluginGetInterface"

#endif /* __LWNET_PLUGIN_H__ */
