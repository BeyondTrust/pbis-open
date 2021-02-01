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
 *        nss_dbdefs.h
 *
 * Abstract:
 *
 *        Name Server Switch (BeyondTrust LSASS)
 *
 *        Reverse-engineered structures and constants for HP-UX NSS interface
 *        
 *        Authors: Kyle Stemen (kstemen@likewisesoftware.com)
 *
 */


#include <nsswitch.h>
#include <prot.h>

struct nss_backend;

typedef int nss_status_t;

#define NSS_SUCCESS     __NSW_SUCCESS
#define NSS_UNAVAIL     __NSW_UNAVAIL
#define NSS_TRYAGAIN    __NSW_TRYAGAIN
#define NSS_NOTFOUND    __NSW_NOTFOUND

typedef nss_status_t (*nss_function)(struct nss_backend *be, void *args);

typedef struct nss_backend
{
    nss_function *ops;
    int n_ops; 
} nss_backend_t;

typedef struct
{
    struct
    {
        void *result;
        char *buffer;
        size_t buflen;
    } buf;
    
    char *unknown1;
    char *unknown2;
    
    union
    {
        char    *name;
        uid_t   uid;
        gid_t   gid;
    } key;
    
    int unknown3;
    int unknown4;
    
    void *returnval;
    int erange;
} nss_XbyY_args_t;

struct nss_groupsbymem
{
    const char *username;
    gid_t *gid_array;
    int maxgids;
    int unknown1;
    void *unknown2;
    void *unknown3;
    int numgids;
};
