#include "gpagent.h"

#define USE_40 1

/* work around problem in Solaris headers */
#if defined(__LWI_SOLARIS__)
#define _OPTIONAL_IS_INT_CAST_ (int)
#else
#define _OPTIONAL_IS_INT_CAST_
#endif

#define ISSPACE(x) isspace(_OPTIONAL_IS_INT_CAST_ x)
#define ISXDIGIT(x) isxdigit(_OPTIONAL_IS_INT_CAST_ x)
#define ISPRINT(x) isprint(_OPTIONAL_IS_INT_CAST_ x)

int
GetBufferFromHexString(
    /* OUT */ void** Buffer,
    /* OUT */ size_t* Length,
    /* IN */ const char* HexString
    )
{
    int error = 0;
    char byte[3] = { 0 };
    size_t byteIndex = 0;
    unsigned char* outputBuffer = NULL;
    size_t index = 0;
    size_t length = strlen(HexString);
    size_t digitCount = 0;
    size_t outputIndex = 0;

    outputBuffer = malloc(length);
    if (!outputBuffer)
    {
        error = ENOMEM;
        goto cleanup;
    }

    while (index < length)
    {
        while (ISSPACE(HexString[index]))
        {
            index++;
        }

        if (('0' == HexString[index]) &&
            (('x' == HexString[index]) ||
             ('X' == HexString[index])))
        {
            index += 2;
            continue;
        }

        if (!ISXDIGIT(HexString[index]))
        {
            if (ISPRINT(HexString[index]))
            {
                printf("Bad hex digit '%c'\n", HexString[index]);
            }
            else
            {
                printf("Bad hex digit with value %d\n", HexString[index]);
            }
            error = EINVAL;
            goto cleanup;
        }

        byte[byteIndex] = HexString[index];
        byteIndex++;

        if (byteIndex == (sizeof(byte)-1))
        {
            unsigned long value = strtoul(byte, NULL, 16);
            outputBuffer[outputIndex] = (unsigned char)value;
            outputIndex++;
            byteIndex = 0;
        }

        digitCount++;
        index++;
    }

    /* If odd, something is wrong */
    if (byteIndex != 0)
    {
        printf("Odd number of digits (%zd)\n", digitCount);
        error = EINVAL;
        goto cleanup;
    }

 cleanup:
    if (error)
    {
        if (outputBuffer)
        {
            free(outputBuffer);
            outputBuffer = NULL;
            outputIndex = 0;
        }
    }

    *Buffer = outputBuffer;
    *Length = outputIndex;

    return error;
}


int main(int argc, const char* argv[])
{
    CENTERROR ceError = 0;
    int error = 0;
    dns_srv_info* serverList = NULL;
    void* buffer = NULL;
    size_t len = 0;
    char input1[] = "ae9685800001000200040006055f6c646170045f74637004544553540651494e54524103434f4d00\
00210001c00c002100010000025800220000006401850a4e544f524c444354303404746573740651\
494e54524103434f4d00c00c002100010000025800220000006401850a6e746f726c646374303304\
746573740651494e54524103434f4d00c0170002000100000e10001509444e5652444e5330320571\
77657374036e657400c0170002000100000e10000805444e533031c09ec0170002000100000e1000\
0c094f4d4148444e533031c09ec0170002000100000e10000c0944424c4e444e533031c09e0a4e54\
4f524c4443543034c01700010001000004b000040a001c140a6e746f726c6463743033c017000100\
010000038400040a001c15c094000100010000038400040a0000fdc0b5000100010000038400040a\
0000fac0c9000100010000038400040a060264c0e1000100010000038400040a080863";
    char input2[] = "000485800001000100000001055f6c646170045f74637004636f72700863656e746572697303636f6d0000210001c00c0021000100000258001e000000640185047372763204636f72700863656e746572697303636f6d00c0400001000100000e1000040a640118";
    char* input = input1;
    dns_srv_info* server;

    if (argc > 1)
    {
        input = input2;
    }

    (void) gpa_init_logging_to_file(4, NULL);

    error = GetBufferFromHexString(&buffer, &len, input);
    if (error)
    {
        return 1;
    }

    printf("len = %zd\n", len);

#if USE_40
    ceError = GetDNSServerListEx(buffer, len, "nothing", &serverList);
#else
    ceError = GetDomainControllerListEx(buffer, len, "nothing", &serverList);
#endif
    if (ceError)
    {
        printf("got ceerror = 0x%08x\n", ceError);
        return 1;
    }
#define SAFE_STR(x) ((x) ? (x) : "(null)")
    for (server = serverList; server; server = server->next)
    {
        printf("%s -> %s\n", SAFE_STR(server->target), SAFE_STR(server->address));
    }
    printf("%p", serverList);
    return 0;
}
