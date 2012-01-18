/*
 * Copyright (c) Likewise Software.  All rights reserved.
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

#define EVT_DEFAULT_MAX_LOG_SIZE    104857600 // 100M, int bytes
#define EVT_DEFAULT_MAX_RECORDS     100000
#define EVT_DEFAULT_MAX_AGE         90 // days
#define EVT_DEFAULT_PURGE_INTERVAL  1 // days


#define EVT_BYTES_IN_KB 	(1024)
#define EVT_BYTES_IN_MB   	(1024 * EVT_BYTES_IN_KB)
#define EVT_BYTES_IN_GB    	(1024 * EVT_BYTES_IN_MB)

#define EVT_RECS_IN_K 		(1000)
#define EVT_RECS_IN_M   	(1000 * EVT_RECS_IN_K)

