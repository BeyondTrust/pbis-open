#include <stdio.h>
#include "config.h"
#include "lwnet-system.h"
#include "lwnet-def.h"
#include "lwnet.h"
#include "lwnet-utils.h"
#include "lwnet-netbios.h"


int main(int argc, char *argv[])
{
    char *str = NULL;

    unsigned char nbBuf[33] = {0};
    char tmpbuf[33] = {0};

    if (argc == 1)
    {
        printf("usage: %s NETBIOS_NAME\n", argv[0]);
        return 1;
    }
    str = argv[1];

    LWNetNbStrToNbName(str, 0, nbBuf);
    printf("NBName= '%s'\n", nbBuf);

    LWNetNbNameToStr(nbBuf, tmpbuf, NULL);
    printf("InName= '%s'\n", tmpbuf);

    return 0;
}
