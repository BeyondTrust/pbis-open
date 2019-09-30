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

#define LOG_ERROR(Format, ...) \
    fprintf(stderr, Format, ## __VA_ARGS__)

#define BAIL_ON_UP_ERROR(dwError ) \
    if ( dwError ) { \
      goto error;    \
    }

#define BAIL_ON_INVALID_POINTER(pParam) \
    if (!pParam) { \
        dwError = LW_ERROR_INVALID_PARAMETER; \
        goto error; \
    }

#define BAIL_ON_INVALID_STRING(pszParam) \
    if ( LW_IS_NULL_OR_EMPTY_STR(pszParam)) { \
        dwError = LW_ERROR_INVALID_PARAMETER; \
        goto error; \
    }

#define UP_SECONDS_IN_MINUTE (60)
#define UP_SECONDS_IN_HOUR   (60 * UP_SECONDS_IN_MINUTE)
#define UP_SECONDS_IN_DAY    (24 * UP_SECONDS_IN_HOUR)

