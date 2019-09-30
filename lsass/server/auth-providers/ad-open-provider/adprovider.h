/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        adprovider.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Active Directory Provider
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

//#define AD_CACHE_IN_MEMORY

#include "config.h"
#include "lsasystem.h"
#include "lsadef.h"
#include "lsa/lsa.h"
#include "lsajoin.h"
#include "lsa/lsapstore-types.h"
#include "lsa/lsapstore-api.h"

#include <openssl/md4.h>
#include <openssl/rand.h>
#include <openssl/des.h>
#include <sqlite3.h>
#include <uuid/uuid.h>
#include <lwnet.h>
#include <lwio/lwio.h>
#include <reg/lwreg.h>
#include <reg/regutil.h>

#include "lwmem.h"
#include "lwstr.h"
#include "lwhash.h"
#include "lwfile.h"
#include <lwkrb5.h>
#include <lw/rpc/samr.h>
#include <lw/rpc/netlogon.h>
#include <lw/rpc/krb5pac.h>
#include "lwsecurityidentifier.h"
#include "lsautils.h"
#include "lsaipc.h"
#include <lsa/provider.h>
#include "lsasrvapi.h"
#include "lsaadprovider.h"

#include "lsasrvutils.h"
#include "lsasrvcred.h"
#include "lwldap.h"
#include "lsaldap.h"

#include "addef.h"
#include "media-sense.h"
#include "adstruct.h"
#include "adcache.h"
#include "adcachespi.h"
#include "adcfg.h"
#include "adldapdef.h"
#include "adnetapi.h"
#include "lsadm.h"
#include "lsadmengine.h"
#include "lsadmwrap.h"
#include "lsaum.h"
#include "lsaumproc.h"
#include "state_store.h"
#include "adldap.h"
#include "adldap_p.h"
#include "batch.h"
#include "unprov.h"
#include "batch_marshal.h"
#include "ad_marshal_group.h"
#include "ad_marshal_nss_artefact.h"
#include "ad_marshal_nss_artefact_p.h"
#include "ad_marshal_user.h"
#include "ad_marshal_user_p.h"
#include "cellldap.h"
#include "defldap.h"
#include "enumstate.h"
#include "machinepwd_p.h"
#include "offline.h"
#include "online.h"
#include "providerstate.h"
#include "provider-main.h"
#include "offline-helper.h"
#include "lsasqlite.h"
#include "lsasqlite_p.h"
#include "pwdcache_p.h"
#include "ioctl.h"
#include "mount.h"

#include "externs.h"

#include "sqlcache_create.h"
#include "sqlcache_p.h"
#include "memcache_p.h"
#include "specialdomain.h"
#include "lsakrb5smb.h"

