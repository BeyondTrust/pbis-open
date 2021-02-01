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

#ifndef _EXTERNS_H_
#define _EXTERNS_H_

extern pthread_mutex_t gNetapiDataMutex;


#define GLOBAL_DATA_LOCK(locked)                         \
    do {                                                 \
        int ret = 0;                                     \
        ret = pthread_mutex_lock(&g_netapi_data_mutex);  \
        if (ret) {                                       \
            status = STATUS_UNSUCCESSFUL;                \
            goto error;                                  \
	    	                                             \
        } else {                                         \
            locked = 1;                                  \
        }                                                \
    } while (0);


#define GLOBAL_DATA_UNLOCK(locked)                        \
    do {                                                  \
        int ret = 0;                                      \
        if (!locked) break;                               \
        ret = pthread_mutex_unlock(&g_netapi_data_mutex); \
        if (ret && status == STATUS_SUCCESS) {            \
            status = STATUS_UNSUCCESSFUL;                 \
                                                          \
        } else {                                          \
            locked = 0;                                   \
        }                                                 \
    } while (0);


extern int errno;


#endif /* _EXTERN_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
