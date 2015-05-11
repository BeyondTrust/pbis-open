#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "config.h"
#include "lwnet-system.h"
#include "lwnet-def.h"
#include "lwnet.h"
#include "lwnet-utils.h"
#include "lwnet-netbios.h"


int main(int argc, char *argv[])
{
    int i = 0;
    PSTR str = NULL;
    PSTR fromNbName2ToString = NULL;
    PBYTE nbName2 = NULL;
    DWORD nbName2Len = 0;
    char **nbNameParts = NULL;
    DWORD nbNamePartsLen = 0;
    int sts = 0;
    UINT8 searchType = 0;
    UINT8 queryType = 0;
    DWORD nbNameBufSize = 0;

    if (argc == 1)
    {
        printf("usage: %s NETBIOS_NAME\n", argv[0]);
        return 1;
    }
    str = argv[1];
    if (argc > 2)
    {
        queryType = strtol(argv[2], NULL, 0);
    }

    sts = LWNetNbStrToNbName2(
              str,
              queryType,
              &nbName2,
              &nbName2Len);
    if (sts)
    {
        printf("NbNameToStr2: failed %d\n", sts);
        return 1;
    }

    sts = LWNetNbName2ToParts(
              nbName2,
              &nbNameParts,
              &nbNamePartsLen);
    if (sts == 0)
    {
        for (i=0; nbNameParts[i]; i++)
        {
            printf("nbNamePart[%d] = '%s'\n", i, nbNameParts[i]);
            free(nbNameParts[i]);
        }
        free(nbNameParts);
    }

    sts = LWNetNbName2ToStr(
              nbName2,
              &fromNbName2ToString,
              &searchType,
              &nbNameBufSize);
    if (sts == 0)
    {
        printf("NetBIOS name = '%s'\n", fromNbName2ToString);
        printf("NetBIOS search = '0x%02x'\n", (unsigned int) searchType);
        printf("NetBIOS buflen = '%d'\n", nbNameBufSize);
    }

    if (fromNbName2ToString)
    {
        free(fromNbName2ToString);
    }
    if (nbName2)
    {
        free(nbName2);
    }

    return 0;
}
