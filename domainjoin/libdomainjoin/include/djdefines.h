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

#ifndef __DJDEFINES_H__
#define __DJDEFINES_H__

#define MAX_PROC_BUF_LEN 1024

#if 0
#ifdef BAIL_ON_CENTERIS_ERROR
#undef BAIL_ON_CENTERIS_ERROR
#endif

//Copying ceError into a variable allows ceError to be a function call
//The do while forces the user to put a semicolon after BAIL_ON_CENTERIS_ERROR
#define BAIL_ON_CENTERIS_ERROR(ceError)                                 \
    do {                                                                \
        DWORD macroCeError = (ceError);                             \
        if (macroCeError) {                                             \
            DJ_LOG_ERROR("Error at %s:%d. Error code [0x%8x]", __FILE__, __LINE__, macroCeError); \
            goto error;                                                 \
        }                                                               \
    } while(0)
#endif

#endif /* __DJDEFINES_H__ */
