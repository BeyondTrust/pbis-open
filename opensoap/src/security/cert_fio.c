/*-----------------------------------------------------------------------------
 * $RCSfile: cert_fio.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include "security_defs.h"
#include <string.h>

static unsigned char s_aucVer[4] = {
    (unsigned char)0, (unsigned char)0, (unsigned char)0, (unsigned char)1
};
#define PUBKEY_LINE_LENGTH (64)

/***************************************************************************** 
    Function      : Serialize WORD value
    Return        : void
 ************************************************ Yuji Yamawaki 02.02.06 *****/
static void serializeWORD
(unsigned short wData,
 unsigned char* szArea)
{
    int i;
    for (i = 0; i < 2; i++) {
        *szArea = (unsigned char)(wData >> 8 * (1 - i));
        szArea++;
    } 
}
/***************************************************************************** 
    Function      : deserialize WORD value
    Return        : unsigned short
 ************************************************ Yuji Yamawaki 02.02.07 *****/
static unsigned short  deserializeWORD
(const unsigned char* szArea)
{
    unsigned short usRet = 0;
    int i;

    for (i = 0; i < 2; i++) {
        usRet <<= 8;
        usRet |= szArea[i];
    } 
    return usRet;
}
/***************************************************************************** 
    Function      : Serialize DWORD value
    Return        : void
 ************************************************ Yuji Yamawaki 02.02.06 *****/
static void serializeDWORD
(unsigned long  dwData,
 unsigned char* szArea)
{
    int i;
    for (i = 0; i < 4; i++) {
        *szArea = (unsigned char)(dwData >> 8 * (3 - i));
        szArea++;
    } 
}
/***************************************************************************** 
    Function      : deserialize DWORD value
    Return        : unsigned long
 ************************************************ Yuji Yamawaki 02.02.07 *****/
static unsigned long deserializeDWORD
(const unsigned char* szArea)
{
    unsigned long ulRet = 0;
    int i;

    for (i = 0; i < 4; i++) {
        ulRet <<= 8;
        ulRet |= szArea[i];
    } 
    return ulRet;
}
/***************************************************************************** 
    Function      : Set Former Certificate(No.1-3)
    Return        : int
 ************************************************ Yuji Yamawaki 02.02.06 *****/
static int setFormerCert
(const char*    szPublish,      /* (i)  Publisher's Name */
 unsigned char* szArea)         /* (o)  */
{
    int   nRet = OPENSOAP_NO_ERROR;
    unsigned char* szCurPos;
    unsigned short wData;

    /* Initialize */
    szCurPos = szArea;
    /* Version */
    memcpy(szCurPos, s_aucVer, sizeof(s_aucVer));
    szCurPos += sizeof(s_aucVer);
    /* Field Length */
    wData = strlen(szPublish) + 1;
    serializeWORD(wData, szCurPos);
    szCurPos += sizeof(wData);
    /* Publisher's Name */
    strcpy(szCurPos, szPublish);
    szCurPos += wData;
    return nRet;
}
/***************************************************************************** 
    Function      : Set Latter Certificate(No.7-13)
    Return        : int
 ************************************************ Yuji Yamawaki 02.02.06 *****/
static int setLatterCert
(const OpenSOAPCARecPtr pRec,    /* (i)  CA Record */
 unsigned char*         szArea)  /* (o)  */
{
    int   nRet = OPENSOAP_NO_ERROR;
    unsigned char* szCurPos;
    unsigned short wData;

    /* Initialize */
    szCurPos = szArea;
    /* Serial Number */
    serializeDWORD(pRec->ulSerial, szCurPos);
    szCurPos += sizeof(pRec->ulSerial);
    /* Length of Owner's Name */
    wData = strlen(pRec->szOwner) + 1;
    serializeWORD(wData, szCurPos);
    szCurPos += sizeof(wData);
    /* Owner's Name */
    strcpy(szCurPos, pRec->szOwner);
    szCurPos += wData;
    /* Start Date */
    memcpy(szCurPos, pRec->szStart, sizeof(pRec->szStart));
    szCurPos += sizeof(pRec->szStart);
    /* End Date */
    memcpy(szCurPos, pRec->szEnd, sizeof(pRec->szEnd));
    szCurPos += sizeof(pRec->szEnd);
    /* Length of Public Key */
    serializeWORD(pRec->usKeyLen, szCurPos);
    szCurPos += sizeof(pRec->usKeyLen);
    /* Public Key */
    memcpy(szCurPos, pRec->ucKey, pRec->usKeyLen);
    szCurPos += pRec->usKeyLen;
    return nRet;
} 
/***************************************************************************** 
    Function      : Create Certificate from CA Record(stream)
    Return        : int
 ************************************************ Yuji Yamawaki 02.02.06 *****/
int
OPENSOAP_API
OpenSOAPSecCertCreateWithStream
(const char*            szPublish, /* (i)  Publisher's Name */
 FILE*                  fpPrivKey, /* (i)  Private Key(Publisher) */
 int                    iHashType, /* (i)  Hash Type(OPENSOAP_HA_*) */
 const OpenSOAPCARecPtr pRec,      /* (i)  CA Record */
 FILE*                  fpCert)    /* (o)  Certificate File */
{
    int nRet = OPENSOAP_NO_ERROR;
    size_t sizFormer, sizLatter;
    unsigned char* szAreaS = NULL;
    unsigned char* szDSig = NULL;
    int            nSigLen;
    unsigned char  ucWk;
    unsigned short wWk;
    char           szWk[4];

    /* Check Arguments */
    if (szPublish == NULL || fpPrivKey == NULL ||
        pRec == NULL || fpCert == NULL) {
        nRet = OPENSOAP_PARAMETER_BADVALUE;
        goto FuncEnd;
    }
    /* Calculate Size */
    sizFormer = 4 + 2 + strlen(szPublish) + 1;
    sizLatter = 4 + 2 + strlen(pRec->szOwner) + 1 + 14 + 14 + 2 +
        pRec->usKeyLen;
    /* Allocate area(for signature) */
    szAreaS = (unsigned char*)malloc(sizFormer + sizLatter);
    if (szAreaS == NULL) {
        nRet = OPENSOAP_MEM_BADALLOC;
        goto FuncEnd;
    }
    /* Set Former Data */
    nRet = setFormerCert(szPublish, szAreaS);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Set Latter Data */
    nRet = setLatterCert(pRec, szAreaS + sizFormer);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Sign */
    nRet = openSOAPSecMakeRSABinSign(iHashType,
                                     szAreaS,
                                     sizFormer + sizLatter,
                                     fpPrivKey,
                                     &szDSig,
                                     &nSigLen,
                                     NULL, NULL);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Write To File */
    if (fwrite(szAreaS, sizFormer, 1, fpCert) != 1) {
        nRet = OPENSOAP_IO_WRITE_ERROR;
        goto FuncEnd;
    }
    ucWk = (unsigned char)iHashType;
    if (fwrite(&ucWk, sizeof(ucWk), 1, fpCert) != 1) {
        nRet = OPENSOAP_IO_WRITE_ERROR;
        goto FuncEnd;
    }
    wWk = (unsigned short)nSigLen;
    serializeWORD(wWk, szWk);
    if (fwrite(szWk, 1, sizeof(wWk), fpCert) != sizeof(wWk)) {
        nRet = OPENSOAP_IO_WRITE_ERROR;
        goto FuncEnd;
    }
    if (fwrite(szDSig, nSigLen, 1, fpCert) != 1) {
        nRet = OPENSOAP_IO_WRITE_ERROR;
        goto FuncEnd;
    }
    if (fwrite(szAreaS + sizFormer, sizLatter, 1, fpCert) != 1) {
        nRet = OPENSOAP_IO_WRITE_ERROR;
        goto FuncEnd;
    }

FuncEnd:
    if (szAreaS != NULL) {
        free(szAreaS);
    }
    if (szDSig != NULL) {
        free(szDSig);
    }
    return nRet;
}
/***************************************************************************** 
    Function      : Create Certificate from CA Record(File)
    Return        : int
 ************************************************ Yuji Yamawaki 02.02.06 *****/
int
OPENSOAP_API
OpenSOAPSecCertCreateWithFile
(const char*            szPublish,     /* (i)  Publisher's Name */
 const char*            szPrivKeyFile, /* (i)  PrivKey File Name (Publisher) */
 int                    iHashType,     /* (i)  Hash Type(OPENSOAP_HA_*) */
 const OpenSOAPCARecPtr pRec,          /* (i)  CA Record */
 const char*            szCertName)    /* (i)  Certificate File Name */
{
    int nRet = OPENSOAP_PARAMETER_BADVALUE;
    FILE* fpPrivKey = NULL;     /* Private Key File Stream */
    FILE* fpCert    = NULL;     /* Certificate File Stream */
    /* Open Private Key File */
    fpPrivKey = fopen(szPrivKeyFile, "rb");
    if (fpPrivKey == NULL) {
        nRet = OPENSOAP_FILEOPEN_ERROR;
        goto FuncEnd;
    }
    /* Open Certificate File */
    fpCert = fopen(szCertName, "wb");
    if (fpCert == NULL) {
        nRet = OPENSOAP_FILEOPEN_ERROR;
        goto FuncEnd;
    }
    /* Create Certificate */
    nRet = OpenSOAPSecCertCreateWithStream(szPublish,
                                           fpPrivKey,
                                           iHashType,
                                           pRec,
                                           fpCert);
FuncEnd:
    if (fpPrivKey != NULL) {
        fclose(fpPrivKey);
    }
    if (fpCert != NULL) {
        fclose(fpCert);
    }
    return nRet;
}
/***************************************************************************** 
    Function      : Load Sign Check Area
    Return        : int
 ************************************************ Yuji Yamawaki 02.02.07 *****/
static int loadSigCheckArea
(FILE*           fpCert,        /* (i)  Certificate File Stream */
 int*            piHashType,    /* (o)  Hash Type */
 unsigned char** ppucArea,      /* (o)  Check Area*/
 size_t*         pSizArea,      /* (o)  Length of Check Area*/
 unsigned char** ppucSig,       /* (o)  Signature */
 size_t*         pSizSig)       /* (o)  Length of Signature */
{
    int nRet = OPENSOAP_NO_ERROR;
    unsigned char* pucNew = NULL;
    unsigned char* pucWk; /* Working Area */
    size_t         sizArea = 0;
    size_t         sizRead;
    unsigned short usLen;
    unsigned char  aucTmp[2];

    /* Initialize */
    *ppucSig = NULL;
    /* Version ID */
    sizRead = 4;
    pucWk = (unsigned char*)realloc(pucNew, sizArea + sizRead);
    if (pucWk == NULL) {
        nRet = OPENSOAP_MEM_BADALLOC;
        goto FuncEnd;
    }
    pucNew = pucWk;
    if (fread(pucNew + sizArea, sizRead, 1, fpCert) != 1) {
        nRet = OPENSOAP_IO_READ_ERROR;
        goto FuncEnd;
    }
    sizArea += sizRead;
    /* Field Length */
    sizRead = 2;
    pucWk = (unsigned char*) realloc(pucNew, sizArea + sizRead);
    if (pucWk == NULL) {
        nRet = OPENSOAP_MEM_BADALLOC;
        goto FuncEnd;
    }
    pucNew = pucWk;
    if (fread(pucNew + sizArea, sizRead, 1, fpCert) != 1) {
        nRet = OPENSOAP_IO_READ_ERROR;
        goto FuncEnd;
    }
    usLen = deserializeWORD(pucNew + sizArea);
    sizArea += sizRead;
    /* Publisher Name */
    sizRead = (size_t)usLen;
    pucWk = (unsigned char*) realloc(pucNew, sizArea + sizRead);
    if (pucWk == NULL) {
        nRet = OPENSOAP_MEM_BADALLOC;
        goto FuncEnd;
    }
    pucNew = pucWk;
    if (fread(pucNew + sizArea, sizRead, 1, fpCert) != 1) {
        nRet = OPENSOAP_IO_READ_ERROR;
        goto FuncEnd;
    }
    sizArea += sizRead;
    /* Hash Algorithm (Not In Check Area) */
    if (fread(aucTmp, 1, 1, fpCert) != 1) {
        nRet = OPENSOAP_IO_READ_ERROR;
        goto FuncEnd;
    }
    *piHashType = (int)(aucTmp[0]);
    /* Signature Length (Not In Check Area) */
    if (fread(aucTmp, sizeof(aucTmp), 1, fpCert) != 1) {
        nRet = OPENSOAP_IO_READ_ERROR;
        goto FuncEnd;
    }
    *pSizSig = (size_t)deserializeWORD(aucTmp);
    /* Signature (Not In Check Area) */
    *ppucSig = (unsigned char*)malloc(*pSizSig);
    if (*ppucSig == NULL) {
        nRet = OPENSOAP_MEM_BADALLOC;
        goto FuncEnd;
    }
    if (fread(*ppucSig, *pSizSig, 1, fpCert) != 1) {
        nRet = OPENSOAP_IO_READ_ERROR;
        goto FuncEnd;
    }
    /* Serial Number */
    sizRead = 4;
    pucWk = (unsigned char*) realloc(pucNew, sizArea + sizRead);
    if (pucWk == NULL) {
        nRet = OPENSOAP_MEM_BADALLOC;
        goto FuncEnd;
    }
    pucNew = pucWk;
    if (fread(pucNew + sizArea, sizRead, 1, fpCert) != 1) {
        nRet = OPENSOAP_IO_READ_ERROR;
        goto FuncEnd;
    }
    sizArea += sizRead;
    /* Field Length */
    sizRead = 2;
    pucWk = (unsigned char*) realloc(pucNew, sizArea + sizRead);
    if (pucWk == NULL) {
        nRet = OPENSOAP_MEM_BADALLOC;
        goto FuncEnd;
    }
    pucNew = pucWk;
    if (fread(pucNew + sizArea, sizRead, 1, fpCert) != 1) {
        nRet = OPENSOAP_IO_READ_ERROR;
        goto FuncEnd;
    }
    usLen = deserializeWORD(pucNew + sizArea);
    sizArea += sizRead;
    /* Owner Name */
    sizRead = (size_t)usLen;
    pucWk = (unsigned char*) realloc(pucNew, sizArea + sizRead);
    if (pucWk == NULL) {
        nRet = OPENSOAP_MEM_BADALLOC;
        goto FuncEnd;
    }
    pucNew = pucWk;
    if (fread(pucNew + sizArea, sizRead, 1, fpCert) != 1) {
        nRet = OPENSOAP_IO_READ_ERROR;
        goto FuncEnd;
    }
    sizArea += sizRead;
    /* Date * 2 */
    sizRead = 14 * 2;
    pucWk = (unsigned char*) realloc(pucNew, sizArea + sizRead);
    if (pucWk == NULL) {
        nRet = OPENSOAP_MEM_BADALLOC;
        goto FuncEnd;
    }
    pucNew = pucWk;
    if (fread(pucNew + sizArea, sizRead, 1, fpCert) != 1) {
        nRet = OPENSOAP_IO_READ_ERROR;
        goto FuncEnd;
    }
    sizArea += sizRead;
    /* Field Length */
    sizRead = 2;
    pucWk = (unsigned char*) realloc(pucNew, sizArea + sizRead);
    if (pucWk == NULL) {
        nRet = OPENSOAP_MEM_BADALLOC;
        goto FuncEnd;
    }
    pucNew = pucWk;
    if (fread(pucNew + sizArea, sizRead, 1, fpCert) != 1) {
        nRet = OPENSOAP_IO_READ_ERROR;
        goto FuncEnd;
    }
    usLen = deserializeWORD(pucNew + sizArea);
    sizArea += sizRead;
    /* Public Key */
    sizRead = (size_t)usLen;
    pucWk = (unsigned char*) realloc(pucNew, sizArea + sizRead);
    if (pucWk == NULL) {
        nRet = OPENSOAP_MEM_BADALLOC;
        goto FuncEnd;
    }
    pucNew = pucWk;
    if (fread(pucNew + sizArea, sizRead, 1, fpCert) != 1) {
        nRet = OPENSOAP_IO_READ_ERROR;
        goto FuncEnd;
    }
    sizArea += sizRead;

FuncEnd:
    if (OPENSOAP_FAILED(nRet)) {
        if (pucNew != NULL) {
            free(pucNew);
            pucNew = NULL;
        }
        if (*ppucSig != NULL) {
            free(*ppucSig);
            *ppucSig = NULL;
        }
    }
    *ppucArea = pucNew;
    *pSizArea = sizArea;
    return nRet;
}
/***************************************************************************** 
    Function      : Free Cretificate
    Return        : int
 ************************************************ Yuji Yamawaki 02.02.07 *****/
int
OPENSOAP_API
OpenSOAPSecCertFree
(OpenSOAPSecCertPtr pCert)
{
    int nRet = OPENSOAP_NO_ERROR;
    if (pCert == NULL) {
        goto FuncEnd;
    }
    if (pCert->szPublisher != NULL) {
        free(pCert->szPublisher);
    }
    if (pCert->pucSign != NULL) {
        free(pCert->pucSign);
    }
    if (pCert->pucPubKey != NULL) {
        free(pCert->pucPubKey);
    }
FuncEnd:
    return nRet;
}
/***************************************************************************** 
    Function      : Load from memory
    Return        : int
 ************************************************ Yuji Yamawaki 02.02.07 *****/
int
OPENSOAP_API
OpenSOAPSecCertLoadFromMem
(size_t               sizArea,  /* (i)  Size of Input Area */
 const unsigned char* pucArea,  /* (i)  Input Area */
 OpenSOAPSecCertPtr*  ppCert)   /* (o)  Certificate */
{
    OpenSOAPSecCertPtr pCert = NULL;
    int                nRet = OPENSOAP_NO_ERROR;
    const unsigned char* pucCur;
    unsigned short       usVal;

    /*=== Allocate ===*/
    pCert = (OpenSOAPSecCertPtr)calloc(1, sizeof(struct tagOpenSOAPSecCert));
    if (pCert == NULL) {
        nRet = OPENSOAP_MEM_BADALLOC;
        goto FuncEnd;
    }
    /*=== Load ===*/
    pucCur = pucArea;
    /* Versions */
    memcpy(pCert->aucVersion, pucCur, sizeof(pCert->aucVersion));
    pucCur += sizeof(pCert->aucVersion);
    /* Name of publisher */
    usVal = deserializeWORD(pucCur);
    pucCur += sizeof(usVal);
    pCert->szPublisher = (char*)malloc(usVal);
    if (pCert->szPublisher == NULL) {
        nRet = OPENSOAP_MEM_BADALLOC;
        goto FuncEnd;
    }
    memcpy(pCert->szPublisher, pucCur, usVal);
    pucCur += usVal;
    /* Hash Type */
    pCert->iHashType = (int)(*pucCur);
    pucCur++;
    /* Sign Length */
    pCert->nLenSign = (int)deserializeWORD(pucCur);
    pucCur += 2;
    /* Sign */
    pCert->pucSign = (unsigned char*)malloc(pCert->nLenSign);
    if (pCert->pucSign == NULL) {
        nRet = OPENSOAP_MEM_BADALLOC;
        goto FuncEnd;
    }
    memcpy(pCert->pucSign, pucCur, pCert->nLenSign);
    pucCur += pCert->nLenSign;
    /* Serial Number */
    pCert->ulSerial = deserializeDWORD(pucCur);
    pucCur += 4;
    /* Name of Owner */
    usVal = deserializeWORD(pucCur);
    pucCur += sizeof(usVal);
    pCert->szOwner = (char*)malloc(usVal);
    if (pCert->szOwner == NULL) {
        nRet = OPENSOAP_MEM_BADALLOC;
        goto FuncEnd;
    }
    memcpy(pCert->szOwner, pucCur, usVal);
    pucCur += usVal;
    /* Begin Date */
    memcpy(pCert->szBeginDate, pucCur, sizeof(pCert->szBeginDate));
    pucCur += sizeof(pCert->szBeginDate);
    /* End Date */
    memcpy(pCert->szEndDate, pucCur, sizeof(pCert->szEndDate));
    pucCur += sizeof(pCert->szBeginDate);
    /* Length of public key */
    pCert->nLenPubKey = (int)deserializeWORD(pucCur);
    pucCur += 2;
    /* Public Key */
    pCert->pucPubKey = (unsigned char*)malloc(pCert->nLenPubKey);
    if (pCert->pucPubKey == NULL) {
        nRet = OPENSOAP_MEM_BADALLOC;
        goto FuncEnd;
    }
    memcpy(pCert->pucPubKey, pucCur, pCert->nLenPubKey);
    pucCur += pCert->nLenPubKey;

FuncEnd:
    if (OPENSOAP_FAILED(nRet)) {
        OpenSOAPSecCertFree(pCert);
        pCert = NULL;
    }
    *ppCert = pCert;
    return nRet;
}
/***************************************************************************** 
    Function      : Load From File
    Return        : int
 ************************************************ Yuji Yamawaki 02.02.07 *****/
int
OPENSOAP_API
OpenSOAPSecCertLoad
(const char*          szName,   /* (i)  File Name */
 OpenSOAPSecCertPtr*  ppCert)   /* (o)  Certificate */
{
    OpenSOAPSecCertPtr pCert = NULL;
    int                nRet = OPENSOAP_NO_ERROR;
    FILE*              fpIn;
    long               lSize;
    unsigned char*     pucArea = NULL;
    /* Check Arguments */
    if (szName == NULL || ppCert == NULL) {
        return OPENSOAP_PARAMETER_BADVALUE;
    }
    /* Open Input File */
    fpIn = fopen(szName, "rb");
    if (fpIn == NULL) {
        nRet = OPENSOAP_FILEOPEN_ERROR;
        goto FuncEnd;
    }
    /* Get Size */
    if (fseek(fpIn, 0, SEEK_END) == -1) {
        nRet = OPENSOAP_IO_READ_ERROR;
        goto FuncEnd;
    }
    lSize = ftell(fpIn);
    if (lSize <= 0) {
        nRet = OPENSOAP_IO_READ_ERROR;
        goto FuncEnd;
    }
    rewind(fpIn);
    /* Allocate All Data Size */
    pucArea = (unsigned char*)malloc(lSize);
    if (pucArea == NULL) {
        nRet = OPENSOAP_MEM_BADALLOC;
        goto FuncEnd;
    }
    /* Load */
    if (fread(pucArea, lSize, 1, fpIn) != 1) {
        nRet = OPENSOAP_IO_READ_ERROR;
        goto FuncEnd;
    }
    /* Save to Structure */
    nRet = OpenSOAPSecCertLoadFromMem((size_t)lSize,
                                      pucArea,
                                      &pCert);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
FuncEnd:
    if (pucArea != NULL) {
        free(pucArea);
    }
    if (fpIn != NULL) {
        fclose(fpIn);
    }
    if (OPENSOAP_FAILED(nRet)) {
        OpenSOAPSecCertFree(pCert);
        pCert = NULL;
    }
    *ppCert = pCert;
    return nRet;
} 
/***************************************************************************** 
    Function      : Verify Certificate(stream)
    Return        : int (On Verify Error, OPENSOAP_SEC_SIGNVERIFY_ERROR)
 ************************************************ Yuji Yamawaki 02.02.07 *****/
int
OPENSOAP_API
OpenSOAPSecCertVerifyWithStream
(FILE* fpCert,                  /* (i)  Certificate File Stream */
 FILE* fpPubKey)                /* (i)  Public Key File Stream */
{
    int nRet = OPENSOAP_NO_ERROR;
    unsigned char* pucCheckArea = NULL;
    size_t         sizCheckArea;
    unsigned char* pucSign = NULL;
    size_t         sizSign;
    fpos_t         posTop;
    int            iHashType;

    /* Check Arguments */
    if (fpCert == NULL || fpPubKey == NULL) {
        nRet = OPENSOAP_PARAMETER_BADVALUE;
        goto FuncEnd;
    }
    /* Save Posotion */
    if (fgetpos(fpCert, &posTop) == -1) {
        nRet = OPENSOAP_IO_READ_ERROR;
        goto FuncEnd;
    }
    /* Load Sign Check Area */
    nRet = loadSigCheckArea(fpCert,
                            &iHashType,
                            &pucCheckArea,
                            &sizCheckArea,
                            &pucSign,
                            &sizSign);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Recover Position */
    if (fsetpos(fpCert, &posTop) == -1) {
        nRet = OPENSOAP_IO_READ_ERROR;
        goto FuncEnd;
    }
    /* check signature */
    nRet = openSOAPSecVerifyRSASignBin(iHashType,
                                       pucSign,
                                       sizSign,
                                       pucCheckArea,
                                       sizCheckArea,
                                       fpPubKey);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
FuncEnd:
    if (pucCheckArea != NULL) {
        free(pucCheckArea);
    }
    if (pucSign != NULL) {
        free(pucSign);
    }
    return nRet;
}
/***************************************************************************** 
    Function      : Verify Certificate(File)
    Return        : int
 ************************************************ Yuji Yamawaki 02.02.07 *****/
int
OPENSOAP_API
OpenSOAPSecCertVerifyWithFile
(const char* szCertName,   /* (i)  Certificate File Name */
 const char* szPubKeyName) /* (i)  Public Key File Name */
{
    int nRet = OPENSOAP_NO_ERROR;
    FILE* fpCert = NULL;
    FILE* fpPubKey = NULL;
    /* Check Parameter */
    if (szCertName == NULL || szPubKeyName == NULL) {
        return OPENSOAP_PARAMETER_BADVALUE;
    }
    /* Open Files */
    fpCert = fopen(szCertName, "rb");
    fpPubKey = fopen(szPubKeyName, "rb");
    if (fpCert == NULL || fpPubKey == NULL) {
        nRet = OPENSOAP_FILEOPEN_ERROR;
        goto FuncEnd;
    }
    /* Verify */
    nRet = OpenSOAPSecCertVerifyWithStream(fpCert, fpPubKey);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
FuncEnd:
    if (fpCert != NULL) {
        fclose(fpCert);
    }
    if (fpPubKey != NULL) {
        fclose(fpPubKey);
    }
    return nRet;
}
/***************************************************************************** 
    Function      : Get Name Of Publisher
    Return        : int
 ************************************************ Yuji Yamawaki 02.02.07 *****/
int
OPENSOAP_API
OpenSOAPSecCertGetPublisherName
(OpenSOAPSecCertPtr pCert,      /* (i)   */
 char**             pszName)    /* (o)  Name (Internal Area) */
{
    /* Check Arguments */
    if (pCert == NULL || pszName == NULL) {
        return OPENSOAP_PARAMETER_BADVALUE;
    }
    *pszName = pCert->szPublisher;
    return OPENSOAP_NO_ERROR;
}
/***************************************************************************** 
    Function      : Get Serial Number
    Return        : int
 ************************************************ Yuji Yamawaki 02.02.07 *****/
int
OPENSOAP_API
OpenSOAPSecCertGetSerialNo
(OpenSOAPSecCertPtr pCert,      /* (i)   */
 unsigned long*     pulSerial)  /* (o)  Serial Number */
{
    /* Check Arguments */
    if (pCert == NULL || pulSerial == NULL) {
        return OPENSOAP_PARAMETER_BADVALUE;
    }
    *pulSerial = pCert->ulSerial;
    return OPENSOAP_NO_ERROR;
}
/***************************************************************************** 
    Function      : Get Name of Owner
    Return        : int
 ************************************************ Yuji Yamawaki 02.02.07 *****/
int
OPENSOAP_API
OpenSOAPSecCertGetOwnerName
(OpenSOAPSecCertPtr pCert,      /* (i)   */
 char**             pszName)    /* (o)  Name (Internal Area) */
{
    /* Check Arguments */
    if (pCert == NULL || pszName == NULL) {
        return OPENSOAP_PARAMETER_BADVALUE;
    }
    *pszName = pCert->szOwner;
    return OPENSOAP_NO_ERROR;
}
/***************************************************************************** 
    Function      : Get End Date
    Return        : int
 ************************************************ Yuji Yamawaki 02.02.07 *****/
int
OPENSOAP_API
OpenSOAPSecCertGetEndDate
(OpenSOAPSecCertPtr pCert,      /* (i)   */
 char**             pszDate)    /* (o)  Date(Internal Area) */
{
    /* Check Arguments */
    if (pCert == NULL || pszDate == NULL) {
        return OPENSOAP_PARAMETER_BADVALUE;
    }
    *pszDate = pCert->szEndDate;
    return OPENSOAP_NO_ERROR;
}
/***************************************************************************** 
    Function      : Get Public Key
    Return        : int
 ************************************************ Yuji Yamawaki 02.02.07 *****/
int
OPENSOAP_API
OpenSOAPSecCertGetPubKey
(OpenSOAPSecCertPtr pCert,      /* (i)   */
 const char*        szSaveName) /* (i)  Public Key File Name */
{
    int   nRet = OPENSOAP_NO_ERROR;
    FILE* fpOut = NULL;
    const char* szKeyHead = OPENSOAP_PUBKEY_HEAD_STRING;
    const char* szKeyTail = OPENSOAP_PUBKEY_HEAD_STRING;
    char*       szEncStr = NULL;
    int         nLenEncStr;
    int         i;
    int         nColumn;

    /* Check Arguments */
    if (pCert == NULL || szSaveName == NULL) {
        return OPENSOAP_PARAMETER_BADVALUE;
    }
    /* Open Output file */
    fpOut = fopen(szSaveName, "wt");
    if (fpOut == NULL) {
        nRet = OPENSOAP_FILEOPEN_ERROR;
        goto FuncEnd;
    }
    /* Write Header */
    if (fputs(szKeyHead, fpOut) == EOF) {
        nRet = OPENSOAP_IO_WRITE_ERROR;
        goto FuncEnd;
    }
    /* Get Key (Encoded) */
    nRet = openSOAPSecEncByBase64(pCert->pucPubKey,
                                  (unsigned long)pCert->nLenPubKey,
                                  &szEncStr);
    /* Write With New Line */
    nLenEncStr = strlen(szEncStr);
    nColumn = 0;
    for (i = 0; i < nLenEncStr; i++) {
        if (putc(szEncStr[i], fpOut) == EOF) {
            nRet = OPENSOAP_IO_WRITE_ERROR;
            goto FuncEnd;
        }
        nColumn++;
        if (nColumn == PUBKEY_LINE_LENGTH) {
            if (putc('\n', fpOut) == EOF) {
                nRet = OPENSOAP_IO_WRITE_ERROR;
                goto FuncEnd;
            }
            nColumn = 0;
        }
    }
    if (nColumn != 0) {
        if (putc('\n', fpOut) == EOF) {
            nRet = OPENSOAP_IO_WRITE_ERROR;
            goto FuncEnd;
        }
    }
    /* Write Tailer */
    if (fputs(szKeyTail, fpOut) == EOF) {
        nRet = OPENSOAP_IO_WRITE_ERROR;
        goto FuncEnd;
    }
FuncEnd:
    if (szEncStr != NULL) {
        free(szEncStr);
    }
    if (fpOut != NULL) {
        fclose(fpOut);
    }

    return nRet;
}
