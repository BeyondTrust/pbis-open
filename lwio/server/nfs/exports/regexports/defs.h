/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        defs.h
 *
 * Abstract:
 *
 *        Likewise I/O (LWIO) - NFS
 *
 *        Share Repository based on Registry
 *
 *        Defines
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#define REG_KEY_PATH_NFS_SHARES_A \
                "services\\lwio\\parameters\\drivers\\nfs\\shares"

#define REG_KEY_PATH_NFS_SHARES_W \
          {'s','e','r','v','i','c','e','s','\\',         \
           'l','w','i','o','\\',                         \
           'p','a','r','a','m','e','t','e','r','s','\\', \
           'd','r','i','v','e','r','s','\\',             \
           's','r','v','\\',                             \
           's','h','a','r','e','s',0}

#define REG_KEY_PATH_NFS_SHARES_SECURITY_A \
                "services\\lwio\\parameters\\drivers\\nfs\\shares\\security"

#define REG_KEY_PATH_NFS_SHARES_SECURITY_W \
          {'s','e','r','v','i','c','e','s','\\',         \
           'l','w','i','o','\\',                         \
           'p','a','r','a','m','e','t','e','r','s','\\', \
           'd','r','i','v','e','r','s','\\',             \
           's','r','v','\\',                             \
           's','h','a','r','e','s','\\',                 \
           's','e','c','u','r','i','t','y',0}

#define REG_KEY_NAME_PREFIX_W    {'N','a','m','e','=',0}
#define REG_KEY_COMMENT_PREFIX_W {'C','o','m','m','e','n','t','=',0}
#define REG_KEY_PATH_PREFIX_W    {'P','a','t','h','=',0}
#define REG_KEY_SERVICE_PREFIX_W {'S','e','r','v','i','c','e','=',0}
