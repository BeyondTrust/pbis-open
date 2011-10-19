/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 *        lwtime.h
 *
 * Abstract:
 *
 *        Likewise Advanced API (lwadvapi) 
 *                    
 *        Time Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#ifndef _LWTIME_H_
#define _LWTIME_H_


#define LW_SECONDS_IN_HOUR   (60 * LW_SECONDS_IN_MINUTE)
#define LW_SECONDS_IN_DAY    (24 * LW_SECONDS_IN_HOUR)
#define LW_SECONDS_IN_MINUTE (60)

#define LW_WINTIME_TO_NTTIME_REL(winTime)  ((winTime) * 10000000LL)
#define LW_NTTIME_TO_WINTIME_REL(ntTime)   ((ntTime) / 10000000LL)


LW_BEGIN_EXTERN_C

DWORD
LwParseDateString(
    PCSTR  pszTimeInterval,
    PDWORD pdwTimeInterval
    );


DWORD
LwSetSystemTime(
    time_t ttCurTime
    );


DWORD
LwGetCurrentTimeSeconds(
    OUT time_t* pTime
    );


DWORD
LwGetNtTime(
    OUT PULONG64 pntTime
    );


ULONG64
LwWinTimeToNtTime(
    IN DWORD winTime
    );


DWORD
LwNtTimeToWinTime(
    IN ULONG64 ntTime
    );

LW_END_EXTERN_C


#endif /* _LWTIME_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
