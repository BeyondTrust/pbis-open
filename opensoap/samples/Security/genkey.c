/*-----------------------------------------------------------------------------
 * $RCSfile: genkey.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include <OpenSOAP/Security.h>

int main(int argc, char* argv[])
{
    char szNamePrivate[256];
    char szNamePublic[256];
    FILE* fpPrivate = NULL;
    FILE* fpPublic  = NULL;
    int   nRet= 0;
    /* Check Argument */
    if (argc < 3) {
        fprintf(stderr, "Usage: %s seedStr keyHeadStr\n", argv[0]);
        return 1;
    }
    /* Set File Name */
    sprintf(szNamePrivate, "privKey_%s.pem", argv[2]);
    sprintf(szNamePublic, "pubKey_%s.pem", argv[2]);
    /* Generate Keys */
    nRet = OpenSOAPSecGenerateRSAKeysToFile(argv[1],
                                            szNamePrivate,
                                            szNamePublic);
    if (OPENSOAP_FAILED(nRet)) {
        fprintf(stderr, 
                "OpenSOAPSecGenerateRSAKeysToFile() Error(%08x).\n",
                (unsigned int)nRet);
        nRet = 1;
        goto FuncEnd;
    }
FuncEnd:
    if (fpPublic != NULL) {
        fclose(fpPublic);
    }
    if (fpPrivate != NULL) {
        fclose(fpPrivate);
    }
    return nRet;
}
