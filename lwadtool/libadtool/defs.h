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
 * Module Name:
 *
 *        defs.h
 *
 * Abstract:
 *
 *        
 *
 * Authors: Author: CORP\slavam
 * 
 * Created on: Mar 17, 2010
 *
 */

#ifndef _ADTOOL_DEFS_H_
#define _ADTOOL_DEFS_H_

/* TODO: Get version number from the build */
#define ADT_VERSION "1.0.0"
#define ADTOOL  "lw-adtool"

#define ADT_APP_USAGE "[OPTIONS] <ACTION> [ACTION_ARGUMENTS]"
#define ADT_ACTION_HELP "[OPTIONS] (-a |--action) %s <ARGUMENTS>\n\n%s\n\nAcceptable arguments ([X] - required):\n"
#define ADT_APP_ALT_USAGE "\nTry \'--help\' for more information.\n"
#define ADT_APP_HELP_ACTIONS_USAGE ""
#define ADT_APP_HELP_ACTIONS_LIST "\nTry \'--help -a\' for a list of actions.\n"
#define ADT_APP_HELP_ACTION_USAGE "\nTry \'--help -a <ACTION>\' for information on a specific action.\n"

#define ADT_ERROR_MSG_SIZE_MAX 2048

#define LDAP_ATTRS_MAX 5000
#define SID_CHARS_MAX 128

#define ADT_SAFE_LOG_STR(s) ((s)?(s):"(null)")

#define ADT_IS_NULL_OR_EMPTY_STR(str) (!(str) || !(*(str)))

#define ADT_TABLEEND(arg) (arg), NULL, '\0', 0, NULL, 0, NULL, NULL

#define ADT_KRB5_DEFAULT_TKT_LIFE (12 * 60 * 60)

#define GROUP_TYPE_DOMAIN_LOCAL "-2147483644"
#define GROUP_TYPE_GLOBAL       "-2147483646"
#define GROUP_TYPE_UNIVERSAL    "-2147483640"

#define GROUP_TYPE_NAME_DOMAIN_LOCAL "domain-local"
#define GROUP_TYPE_NAME_GLOBAL       "global"
#define GROUP_TYPE_NAME_UNIVERSAL    "universal"

#ifndef LW_ENDIAN_SWAP16

#define LW_ENDIAN_SWAP16(wX)                     \
        ((((UINT16)(wX) & 0xFF00) >> 8) |        \
         (((UINT16)(wX) & 0x00FF) << 8))

#endif

#ifndef ULLONG_MAX
#define ULLONG_MAX  18446744073709551615ULL
#endif

#ifndef LW_ENDIAN_SWAP32

#define LW_ENDIAN_SWAP32(dwX)                    \
        ((((UINT32)(dwX) & 0xFF000000L) >> 24) | \
         (((UINT32)(dwX) & 0x00FF0000L) >>  8) | \
         (((UINT32)(dwX) & 0x0000FF00L) <<  8) | \
         (((UINT32)(dwX) & 0x000000FFL) << 24))

#endif

#ifndef LW_ENDIAN_SWAP64

#define LW_ENDIAN_SWAP64(llX)         \
   (((UINT64)(LW_ENDIAN_SWAP32(((UINT64)(llX) & 0xFFFFFFFF00000000LL) >> 32))) | \
   (((UINT64)LW_ENDIAN_SWAP32(((UINT64)(llX) & 0x00000000FFFFFFFFLL))) << 32))

#endif


#define ADT_BAIL_ON_NULL(p) \
    if (p == NULL) {\
      goto error;\
    }

#define ADT_BAIL_ON_ERROR(dwError) \
    if (dwError) {\
         PrintStderr(\
            appContext,\
            LogLevelError, "Error %d (%s) at %s:%d\n",\
            dwError, AdtGetErrorMsg(dwError),\
            __FILE__, __LINE__);\
      goto error;\
    }

#define ADT_BAIL_ON_KRB_ERROR(ctx, ret) \
    if (ret) {\
      Krb5SetGlobalErrorString(ctx, ret);\
      dwError = ADT_KRB5_ERR_BASE + (DWORD) ret;\
      goto error;\
    }

#define ADT_BAIL_ON_ERROR2(dwError) \
    if (dwError) {\
         PrintStderr(\
            appContext,\
            LogLevelError, "Error %d (%s) at %s:%d\n",\
            dwError, AdtGetErrorMsg(dwError),\
            __FILE__, __LINE__);\
      goto error2;\
    }

#define ADT_BAIL_ON_ERROR_STR(dwError, str) \
    if (dwError) {\
         PrintStderr(\
            appContext,\
            LogLevelError, "Error %d (%s) at %s:%d\n",\
            dwError, str,\
            __FILE__, __LINE__);\
      goto error;\
    }

#define ADT_BAIL_ON_ERROR_NP(dwError) \
    if (dwError) {\
      goto error;\
    }

#define ADT_BAIL_ON_ALLOC_FAILURE(p) \
    do {\
        if(!p) {\
         dwError = ADT_ERR_FAILED_ALLOC; \
         PrintStderr(\
            appContext,\
            LogLevelError, "Error %d (%s) at %s:%d\n",\
            dwError, AdtGetErrorMsg(dwError),\
            __FILE__, __LINE__);\
         goto error;\
       }\
    } while(0)

#define ADT_BAIL_ON_ALLOC_FAILURE_NP(p) \
    do {\
        if(!p) {\
         dwError = ADT_ERR_FAILED_ALLOC; \
         goto error;\
       }\
    } while(0)

#define EXIT_NORMALLY \
    do {\
      goto cleanup;\
    } while(0)

#define RETURN_NORMALLY EXIT_NORMALLY

#define EXIT_ABNORMALLY \
    do {\
      goto error;\
    } while(0)

#endif /* _ADTOOL_DEFS_H_ */
