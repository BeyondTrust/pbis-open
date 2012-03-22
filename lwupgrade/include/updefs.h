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

