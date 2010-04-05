#include "config.h"
#include "macadsys.h"
#include "macadutil.h"
#include "lwutils.h"


static
void
ShowUsage()
{
    printf("Usage: resolver <dns A record to query> <start delay seconds> <retry wait seconds> <number of tests> <gethostbyname | res_query>\n");
    fflush(stdout);
}

DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    PSTR*  ppszTargetFQDN,
    int*   staggerTime,
    int*   waitTime,
    int*   numTests,
    BOOLEAN* pbUseGHBN
    )
{

    DWORD dwError = 0;
    int iArg = 1;
    PSTR pszArg = NULL;
    PSTR pszTargetFQDN = NULL;
    int  iStagger = 0;
    int  iWait = 0;
    int  iNumTest = 0;
    BOOLEAN bUseGHBN = FALSE;

    do {
        pszArg = argv[iArg++];
        if (IsNullOrEmptyString(pszArg))
        {
            break;
        }
        if ((strcmp(pszArg, "--help") == 0) ||
                    (strcmp(pszArg, "-h") == 0))
        {
            ShowUsage();
            exit(0);
        }
        else
        {
            if (iArg == 2)
            {
                dwError = LWAllocateString(pszArg, &pszTargetFQDN);
                BAIL_ON_MAC_ERROR(dwError);
            }

            if (iArg == 3)
            {
                iStagger = atoi(pszArg);
            }

            if (iArg == 4)
            {
                iWait = atoi(pszArg);
            }

            if (iArg == 5)
            {
                iNumTest = atoi(pszArg);
            }

            if (iArg == 6)
            {
                if (!strcmp(pszArg, "gethostbyname"))
                {
                    bUseGHBN = TRUE;
                }
                else if (!strcmp(pszArg, "res_query"))
                {
                    bUseGHBN = FALSE;
                }
                else
                {
                    ShowUsage();
                    exit(0);
                }
            }
        }

    } while (iArg < argc);


    if(IsNullOrEmptyString(pszTargetFQDN) || argc < 6)
    {
        ShowUsage();
        exit(0);
    }

cleanup:
    *ppszTargetFQDN = pszTargetFQDN;
    *staggerTime = iStagger;
    *waitTime = iWait;
    *numTests = iNumTest;
    *pbUseGHBN = bUseGHBN;
    return dwError;

 error:
    LW_SAFE_FREE_STRING(pszTargetFQDN);
    *ppszTargetFQDN = NULL;
    *staggerTime = 0;
    *waitTime = 0;
    *numTests = 0;
    *pbUseGHBN = bUseGHBN;
    goto cleanup;
}

void safePrintString(
        PSTR pszStringName,
        PSTR pszStringValue
        )
{
    if(IsNullOrEmptyString(pszStringName))
    {
        return;
    }
    else if(pszStringValue == NULL)
    {
        printf("%s = <NULL>\n", pszStringName);
        fflush(stdout);
    }
    else if(*pszStringValue == '\0')
    {
        printf("%s = <EMPTY>\n", pszStringName);
        fflush(stdout);
    }
    else
    {
        printf("%s = %s\n", pszStringName, pszStringValue);
        fflush(stdout);
    }
}

#if defined(WORDS_BIGENDIAN)
typedef struct {
    uint16_t qr_message_type   : 1; // 0 = Query, 1 = Response
    uint16_t opcode            : 4; // 0 = Std Qry, 1 = Inverse Qry, 2 = Status Req
    uint16_t auth_answer       : 1; // 1 = response from an authoritative server
    uint16_t truncated         : 1; // 1 = message is longer than allowed
    uint16_t recursion_desired : 1; // 1 = server should persue the request recursively
    uint16_t recursion_avail   : 1; // 1 = recursive querying is available on server
    uint16_t reserved          : 2; // zero
    uint16_t answer_authentic  : 1; // 0 = answer authenticated by server, 1 = answer not authenticated
    uint16_t reply_code        : 4; // 0=No err, 1=Format err, 2=Server failure, 3=Name err, 4=Not impl, 5=Refused
} dns_flag_bits;
#else // Mirror image
typedef struct {
    uint16_t reply_code        : 4; // 0=No err, 1=Format err, 2=Server failure, 3=Name err, 4=Not impl, 5=Refused
    uint16_t answer_authentic  : 1; // 0 = answer authenticated by server, 1 = answer not authenticated
    uint16_t reserved          : 2; // zero
    uint16_t recursion_avail   : 1; // 1 = recursive querying is available on server
    uint16_t recursion_desired : 1; // 1 = server should persue the request recursively
    uint16_t truncated         : 1; // 1 = message is longer than allowed
    uint16_t auth_answer       : 1; // 1 = response from an authoritative server
    uint16_t opcode            : 4; // 0 = Std Qry, 1 = Inverse Qry, 2 = Status Req
    uint16_t qr_message_type   : 1; // 0 = Query, 1 = Response
} dns_flag_bits;
#endif

typedef struct _DNS_RESPONSE_HEADER 
{
    WORD wId;
    union {
        dns_flag_bits B;
        WORD        W;
    } flags;
    WORD wQuestions;
    WORD wAnswers;
    WORD wAuths;
    WORD wAdditionals;
    BYTE data[1];
} DNS_RESPONSE_HEADER, *PDNS_RESPONSE_HEADER;

DWORD
DoDnsQueryTest(
    PCSTR pszQuestion,
    PVOID pBuffer,
    DWORD dwBufferSize
    )
{
    int responseSize =  0;

    // TODO: Add locking for res_init()?  See TODO wrt _res.options below.
    if (res_init() != 0)
    {
        // TODO: Fix error code
        return -1;
    }

    if (dwBufferSize < sizeof(DNS_RESPONSE_HEADER))
    {
        return -2;
    }

    _res.options &= ~(RES_USEVC);
    responseSize = res_query(pszQuestion, ns_c_in, ns_t_srv,
                             (PBYTE)pBuffer, dwBufferSize);
    if (responseSize < 0)
    {
        printf("res_query failed with response size of %d.\n", responseSize);
        if (h_errno)
            printf("res_query failed with h_errno %s\n",
                    h_errno == 1 ? "HOST_NOT_FOUND" :
                    h_errno == 2 ? "TRY_AGAIN" :
                    h_errno == 3 ? "NO_RECOVERY" :
                    h_errno == 4 ? "NO_DATA" :
                    "Unknown");
        fflush(stdout);
        return -3;
    }
    if (responseSize < sizeof(DNS_RESPONSE_HEADER))
    {
        printf("res_query failed with response size of %d, which is too small to be valid.\n", responseSize);
        if (h_errno)
            printf("res_query failed with h_errno %s\n",
                    h_errno == 1 ? "HOST_NOT_FOUND" :
                    h_errno == 2 ? "TRY_AGAIN" :
                    h_errno == 3 ? "NO_RECOVERY" :
                    h_errno == 4 ? "NO_DATA" :
                    "Unknown");
        fflush(stdout);
        return -4;
    }
    if (responseSize > dwBufferSize)
    {
        printf("res_query failed with response size of %d, which was larger than our buffer.\n", responseSize);
        if (h_errno)
            printf("res_query failed with h_errno %s\n",
                    h_errno == 1 ? "HOST_NOT_FOUND" :
                    h_errno == 2 ? "TRY_AGAIN" :
                    h_errno == 3 ? "NO_RECOVERY" :
                    h_errno == 4 ? "NO_DATA" :
                    "Unknown");
        fflush(stdout);
        return -5;
    }

    return 0;
}

int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;

    PSTR pszQuery = NULL;
    int stagger = 0;
    int wait = 0;
    int tests = 0;
    int iter = 0;
    const size_t dwBufferSize = (64 * 1024);
    PVOID pBuffer = NULL;
    struct hostent* pHostEntry = NULL;
    BOOLEAN bUseGHBN = FALSE;

    dwError = LWAllocateMemory(dwBufferSize, &pBuffer);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ParseArgs(argc,
            argv,
            &pszQuery,
            &stagger,
            &wait,
            &tests,
            &bUseGHBN
            );
    BAIL_ON_MAC_ERROR(dwError);

    printf("Starting new test\n");
    fflush(stdout);
    sleep(stagger);

    for (iter = 0; iter < tests; iter++)
    {
        if (bUseGHBN)
        {
            printf("Calling gethostbyname(%s)\n", pszQuery);
            fflush(stdout);
            pHostEntry = gethostbyname(pszQuery);
            if (!pHostEntry)
            {
                dwError = h_errno;
                if (dwError)
                    printf("res_query failed with h_errno %s\n",
                            dwError == 1 ? "HOST_NOT_FOUND" :
                            dwError == 2 ? "TRY_AGAIN" :
                            dwError == 3 ? "NO_RECOVERY" :
                            dwError == 4 ? "NO_DATA" :
                            "Unknown");
                    fflush(stdout);
            }
        }
        else
        {
            printf("Calling res_query(%s)\n", pszQuery);
            fflush(stdout);
            dwError = DoDnsQueryTest(pszQuery, pBuffer, dwBufferSize);
        }

        if (dwError)
        {
            printf("DNS query test failed %d\n", dwError);
            fflush(stdout);
        }
        else
        {
            printf("DNS query test passed\n");
            fflush(stdout);
        }
        sleep(wait);
    }

cleanup:

    LW_SAFE_FREE_STRING(pszQuery);
    return (dwError);

error:

    printf("Failed tests. Error code [%d]\n", dwError);
    fflush(stdout);

    goto cleanup;
}
