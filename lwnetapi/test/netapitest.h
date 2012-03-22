/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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

#ifndef _TESTRPC_H_
#define _TESTRPC_H_

#include <config.h>

#include <lw/ntstatus.h>
#include <lw/winerror.h>
#include <wc16str.h>
#include <wc16printf.h>
#include "params.h"


struct _TEST;
struct _CREDENTIALS;


typedef DWORD (*test_fn)(
    struct _TEST        *pTest,
    PCWSTR               pwszHostname,
    PCWSTR               pwszBindingString,
    struct _CREDENTIALS *pCreds,
    struct _PARAMETER   *pOptions,
    DWORD                dwOptcount
    );


typedef struct _TEST
{
    PCSTR         pszName;
    test_fn       Function;

    struct _TEST *pNext;
} TEST, *PTEST;


typedef struct _CREDENTIALS
{
    struct _KRB5
    {
        PWSTR   pwszPrincipal;
        PWSTR   pwszCredsCache;
    } Krb5;

    struct _NTLM
    {
        PWSTR   pwszDomain;
        PWSTR   pwszWorkstation;
        PWSTR   pwszUsername;
        PWSTR   pwszPassword;
    } Ntlm;
} CREDENTIALS, *PCREDENTIALS;


VOID
AddTest(
    PTEST  pFt,
    PCSTR  pszName,
    test_fn function
    );


VOID SetupNetApiTests(PTEST t);
VOID SetupSamrTests(PTEST t);
VOID SetupLsaTests(PTEST t);
VOID SetupNetlogonTests(PTEST t);
VOID SetupDsrTests(PTEST t);
VOID SetupWkssvcTests(PTEST t);


#define STATUS(a, b)                                                     \
    if((status = (a)) != 0) {                                            \
        printf("[\033[31;1mFAILED\033[30;0m] %s test: %s (status=%d)\n", \
               active_test, (b), status);                                \
        return;                                                          \
    }

extern int verbose_mode;

#define WSIZE(a) ((wcslen(a) + 1) * sizeof(wchar16_t))
#define VERBOSE(a) if(verbose_mode) (a)
#define PASSED() printf("[\033[32;1mPASSED\033[31;0m] %s test\n", active_test)
#define FAILED() printf("[\033[31;1mFAILED\033[0m] %s test: ", active_test)

#define test_fail_if_no_memory(ptr)                                  \
    if ((ptr) == NULL) {                                             \
        ret = false;                                                 \
        printf("Test failed: Couldn't allocate pointer %s\n", #ptr); \
        goto done;                                                   \
    }

#define test_fail(printf_args) {                \
        printf printf_args;                     \
        ret = false;                            \
        goto done;                              \
    }

#define netapi_fail(err) {                                    \
        const char *name = Win32ErrorToName(err);             \
        if (name) {                                           \
            printf("NetApi error: %s (0x%08x)\n", name, err); \
        } else {                                              \
            printf("NetApi error: 0x%08x\n", err);            \
        }                                                     \
        goto done;                                            \
    }

#define rpc_fail(err) {                                     \
        const char *name = NtStatusToName(err);             \
        if (name) {                                         \
            printf("Rpc error: %s (0x%08x)\n", name, err);  \
        } else {                                            \
            printf("Rpc error: 0x%08x\n", err);             \
        }                                                   \
        goto done;                                          \
    }

#define BAIL_ON_WIN_ERROR(err)                              \
    if ((err) != ERROR_SUCCESS) {                           \
        goto error;                                         \
    }

#define BAIL_ON_NT_STATUS(status)                           \
    if ((status) != STATUS_SUCCESS) {                       \
        goto error;                                         \
    }

#define NTSTATUS_IS_OK(status)  ((status) == STATUS_SUCCESS)
#define WINERR_IS_OK(err)       ((err) == ERROR_SUCCESS)


#define TESTINFO(test, host)                                        \
    {                                                               \
        printf("#\n# Test: %s\n#\n\n", test->pszName);              \
        if (verbose_mode) {                                         \
            printf("# Test arguments:\n");                          \
            w16printfw(L"#  hostname: %ws\n", host);                \
        }                                                           \
    }

#define PARAM_INFO_START                                        \
    if (verbose_mode) {                                         \
        printf("# Test parameters:\n");                         \
    }

#define PARAM_INFO(name, type, value)                           \
    do {                                                        \
        if (!verbose_mode) break;                               \
        ParamInfo(name, type, (void*)value);                    \
    } while (0)

#define PARAM_INFO_END                                          \
    if (verbose_mode) {                                         \
        printf("#\n");                                          \
    }


#define DUMP_PTR32(pfx, v)                                      \
    if (verbose_mode) {                                         \
        printf("%s%s = 0x%08x\n", pfx, #v, (unsigned int)(v));  \
    }

#define DUMP_PTR64(pfx, v)                                      \
    if (verbose_mode) {                                         \
        printf("%s%s = 0x%16lx\n", pfx, #v, (unsigned long)(v)); \
    }

#if SIZEOF_LONG_INT == 8
#define DUMP_PTR   DUMP_PTR64
#else
#define DUMP_PTR   DUMP_PTR32
#endif

#define DUMP_WSTR(pfx, v)                           \
    if (verbose_mode) {                             \
        w16printfw(L"%hhs%hhs = \"%ws\"\n", pfx, #v, (v)); \
    }

#define DUMP_INT(pfx, v)                                    \
    if (verbose_mode) {                                     \
        printf("%s%s = %d (0x%08x)\n", pfx, #v, (v), (v));  \
    }

#define DUMP_UINT(pfx, v)                                   \
    if (verbose_mode) {                                     \
        printf("%s%s = %u (0x%08x)\n", pfx, #v, (v), (v));  \
    }

#define DUMP_UNICODE_STRING(pfx, v)                     \
    if (verbose_mode) {                                 \
        wchar16_t *str = GetFromUnicodeString((v));     \
        w16printfw(L"%hhs%hhs = \"%ws\"\n", pfx, #v, str);     \
        SAFE_FREE(str);                                 \
    }

#define DUMP_UNICODE_STRING_EX(pfx, v)                  \
    if (verbose_mode) {                                 \
        wchar16_t *str = GetFromUnicodeStringEx((v));   \
        w16printfw(L"%hhs%hhs = \"%ws\"\n", pfx, #v, str);     \
        SAFE_FREE(str);                                 \
    }


#define DUMP_CUSTOM(pfx, v, fn)                 \
    if (verbose_mode) {                         \
        DUMP_PTR(pfx, v);                       \
        fn;                                     \
    }


#define INPUT_ARG_PTR(v)             DUMP_PTR("> ", (v));
#define INPUT_ARG_WSTR(v)            DUMP_WSTR("> ", (v));
#define INPUT_ARG_INT(v)             DUMP_INT("> ", (v));
#define INPUT_ARG_UINT(v)            DUMP_UINT("> ", (v));
#define INPUT_ARG_UNICODE_STRING(v)  DUMP_UNICODE_STRING("> ", (v));
#define INPUT_ARG_CUSTOM(v, fn)      DUMP_CUSTOM("> ", (v), fn);

#define OUTPUT_ARG_PTR(v)            DUMP_PTR("< ", (v));
#define OUTPUT_ARG_WSTR(v)           DUMP_WSTR("< ", (v));
#define OUTPUT_ARG_INT(v)            DUMP_INT("< ", (v));
#define OUTPUT_ARG_UINT(v)           DUMP_UINT("< ", (v));
#define OUTPUT_ARG_UNICODE_STRING(v) DUMP_UNICODE_STRING("< ", (v));
#define OUTPUT_ARG_CUSTOM(v, fn)     DUMP_CUSTOM("< ", (v), fn);

#define RESULT_WSTR(v)               DUMP_WSTR("=> ", (v));
#define RESULT_INT(v)                DUMP_INT("=> ", (v));
#define RESULT_UINT(v)               DUMP_UINT("=> ", (v));

#define CALL_MSRPC(status, msrpc_call)                             \
    do {                                                           \
        printf("-> %s = ", #msrpc_call);                            \
                                                                   \
        status = msrpc_call;                                       \
        printf("%s (0x%08x)\n",                                    \
               NtStatusToName((status)), (status));                \
    } while (0)


#define CALL_NETAPI(status, netapi_call)                           \
    do {                                                           \
        printf("-> %s = ", #netapi_call);                           \
                                                                   \
        status = netapi_call;                                      \
        printf("%s (0x%08x)\n",                                    \
               Win32ErrorToName((status)), (status));              \
    } while (0)


#define DISPLAY_ERROR(msg)                                         \
    do {                                                           \
        printf("%s:%d ", __FILE__, __LINE__);                      \
        printf msg;                                                \
    } while (0)


#define DISPLAY_COMMENT(msg)                                       \
    do {                                                           \
        printf("%s:%d ", __FILE__, __LINE__);                      \
        printf msg;                                                \
    } while (0)


#define ASSERT_TEST(expr)                                          \
    if (!(expr))                                                   \
    {                                                              \
        DISPLAY_ERROR(("assert failed: %s\n", #expr));             \
        bRet = FALSE;                                              \
    }


#define ASSERT_TEST_MSG(expr, msg)                                 \
    if (!(expr))                                                   \
    {                                                              \
        DISPLAY_ERROR(("assert failed: %s ", #expr));              \
        printf msg;                                                \
        bRet = FALSE;                                              \
    }

#define ASSERT_DWORD_EQUAL(val1, val2)                             \
    if ((val1) != (val2))                                          \
    {                                                              \
        DISPLAY_ERROR(("assert failed: %s != %s\n",                \
                       #val1, #val2));                             \
        bRet = FALSE;                                              \
    }


#define ASSERT_WC16STRING_EQUAL(str1_ptr, str2_ptr)                \
    if (((str1_ptr) == NULL && (str2_ptr) != NULL) ||              \
        ((str1_ptr) != NULL && (str2_ptr) == NULL) ||              \
        (((str1_ptr) != (str2_ptr)) &&                             \
         !LwRtlWC16StringIsEqual((str1_ptr), (str2_ptr), FALSE)))  \
    {                                                              \
        DISPLAY_ERROR(("assert failed: %s != %s\n",                \
                       #str1_ptr, #str2_ptr));                     \
        bRet = FALSE;                                              \
    }


#define ASSERT_UNICODE_STRING_EQUAL(str1_ptr, str2_ptr)            \
    if (((str1_ptr) != (str2_ptr)) &&                              \
        !LwRtlUnicodeStringIsEqual((str1_ptr), (str2_ptr),         \
                                   FALSE))                         \
    {                                                              \
        DISPLAY_ERROR(("assert failed: %s != %s\n",                \
                       #str1_ptr, #str2_ptr));                     \
        bRet = FALSE;                                              \
    }


#define ASSERT_UNICODE_STRING_VALID_MSG(name_ptr, msg)             \
    do {                                                           \
        DWORD _iWC = 0;                                            \
                                                                   \
        if (!((name_ptr)->Length == (name_ptr)->MaximumLength) &&  \
            !((name_ptr)->Length + sizeof((name_ptr)->Buffer[0])   \
              == (name_ptr)->MaximumLength))                       \
        {                                                          \
            DISPLAY_ERROR(("assert failed: "                       \
                           "invalid unicode name len/size\n"));    \
            printf msg;                                            \
            bRet = FALSE;                                          \
        }                                                          \
                                                                   \
        if ((name_ptr)->MaximumLength == 0 &&                      \
            (name_ptr)->Buffer != NULL)                            \
        {                                                          \
            DISPLAY_ERROR(("assert failed: "                       \
                           "invalid unicode name "                 \
                           "size == 0 while buffer != NULL\n"));   \
            printf msg;                                            \
            bRet = FALSE;                                          \
        }                                                          \
                                                                   \
        for (_iWC = 0;                                             \
             _iWC < ((name_ptr)->Length                            \
                     / sizeof((name_ptr)->Buffer[0]));             \
             _iWC++)                                               \
        {                                                          \
            if ((name_ptr)->Buffer[_iWC] == 0)                     \
            {                                                      \
                DISPLAY_ERROR(("assert failed: "                   \
                               "invalid unicode name "             \
                               "NULL-termination "                 \
                               "before i < size\n"));              \
                printf msg;                                        \
                bRet = FALSE;                                      \
            }                                                      \
        }                                                          \
                                                                   \
    } while (0)


#define ASSERT_UNICODE_STRING_VALID(name_ptr)                      \
    do {                                                           \
        DWORD _iWC = 0;                                            \
                                                                   \
        if (!((name_ptr)->Length == (name_ptr)->MaximumLength) &&  \
            !((name_ptr)->Length + sizeof((name_ptr)->Buffer[0])   \
              == (name_ptr)->MaximumLength))                       \
        {                                                          \
            DISPLAY_ERROR(("assert failed: "                       \
                           "invalid unicode name len/size\n"));    \
            bRet = FALSE;                                          \
        }                                                          \
                                                                   \
        if ((name_ptr)->MaximumLength == 0 &&                      \
            (name_ptr)->Buffer != NULL)                            \
        {                                                          \
            DISPLAY_ERROR(("assert failed: "                       \
                           "invalid unicode name "                 \
                           "size == 0 while buffer != NULL\n"));   \
            bRet = FALSE;                                          \
        }                                                          \
                                                                   \
        for (_iWC = 0;                                             \
             _iWC < ((name_ptr)->Length                            \
                     / sizeof((name_ptr)->Buffer[0]));             \
             _iWC++)                                               \
        {                                                          \
            if ((name_ptr)->Buffer[_iWC] == 0)                     \
            {                                                      \
                DISPLAY_ERROR(("assert failed: "                   \
                               "invalid unicode name "             \
                               "NULL-termination "                 \
                               "before i < size\n"));              \
                bRet = FALSE;                                      \
            }                                                      \
        }                                                          \
                                                                   \
    } while (0)


#define ASSERT_SID_EQUAL(sid1_ptr, sid2_ptr)                       \
    do {                                                           \
        if ((sid1_ptr) == (sid2_ptr))                              \
        {                                                          \
            break;                                                 \
        }                                                          \
                                                                   \
        if ((sid1_ptr) == NULL && (sid2_ptr) != NULL)              \
        {                                                          \
            DISPLAY_ERROR(("invalid SID: %s == NULL\n",            \
                           #sid1_ptr));                            \
            bRet = FALSE;                                          \
            break;                                                 \
        }                                                          \
                                                                   \
        if ((sid1_ptr) != NULL && (sid2_ptr) == NULL)              \
        {                                                          \
            DISPLAY_ERROR(("invalid SID: %s == NULL\n",            \
                           #sid2_ptr));                            \
            bRet = FALSE;                                          \
            break;                                                 \
        }                                                          \
                                                                   \
        if (!RtlEqualSid((sid1_ptr), (sid2_ptr)))                  \
        {                                                          \
            DISPLAY_ERROR(("assert failed: %s == %s\n",            \
                           #sid1_ptr, #sid2_ptr));                 \
            bRet = FALSE;                                          \
        }                                                          \
    } while (0)


#define ASSERT_SID_EQUAL_MSG(sid1_ptr, sid2_ptr, msg)              \
    do {                                                           \
        if ((sid1_ptr) == (sid2_ptr))                              \
        {                                                          \
            break;                                                 \
        }                                                          \
                                                                   \
        if ((sid1_ptr) == NULL && (sid2_ptr) != NULL)              \
        {                                                          \
            DISPLAY_ERROR(("assert failed: %s == NULL\n",          \
                           #sid1_ptr));                            \
            printf msg;                                            \
            bRet = FALSE;                                          \
            break;                                                 \
        }                                                          \
                                                                   \
        if ((sid1_ptr) != NULL && (sid2_ptr) == NULL)              \
        {                                                          \
            DISPLAY_ERROR(("assert failed: %s == NULL\n",          \
                           #sid2_ptr));                            \
            printf msg;                                            \
            bRet = FALSE;                                          \
            break;                                                 \
        }                                                          \
                                                                   \
        if (!RtlEqualSid((sid1_ptr), (sid2_ptr)))                  \
        {                                                          \
            DISPLAY_ERROR(("assert failed: %s != %s\n",            \
                           #sid1_ptr, #sid2_ptr));                 \
            printf msg;                                            \
            bRet = FALSE;                                          \
        }                                                          \
    } while (0)


#endif /* _TESTRPC_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
