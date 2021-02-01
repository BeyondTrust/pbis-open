/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        lwtime.h
 *
 * Abstract:
 *
 *        BeyondTrust Advanced API (lwadvapi) 
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
