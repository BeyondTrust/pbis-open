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
    unsigned char *ptr = NULL;
    PBYTE nbName2 = NULL;
    DWORD nbName2Len = 0;
    char **nbNameParts = NULL;
    DWORD nbNamePartsLen = 0;
    char nbName1[33] = {0};
    int sts = 0;

    if (argc == 1)
    {
        printf("usage: %s NETBIOS_NAME\n", argv[0]);
        return 1;
    }
    str = argv[1];

    sts = LWNetNbStrToNbName2(
              str,
              0,
              &nbName2,
              &nbName2Len);
    if (sts)
    {
        printf("NbNameToStr2: failed %d\n", sts);
        return 1;
    }

    ptr = nbName2;
    i = (int) *ptr++;

    LWNetNbNameToStr(ptr, nbName1);
    printf("%.*s <%s>\n", i, ptr, nbName1);

    ptr += i;
    while (*ptr)
    {
        i = (int) *ptr++;
        printf("%.*s\n", i, ptr);
        ptr += i;
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
        }
    }

    return 0;
}
