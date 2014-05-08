/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        nss_dbdefs.h
 *
 * Abstract:
 *
 *        Name Server Switch (Likewise LSASS)
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
