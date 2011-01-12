/*-----------------------------------------------------------------------------
 * $RCSfile: CA_database.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include "security_defs.h"
#include <string.h>

/***************************************************************************** 
    Function      : Count Database Record Count
    Return        : int
 ************************************************ Yuji Yamawaki 02.02.05 *****/
static int countRec
(FILE*          fpDb,              /* (i) */
 long*          plRec)             /* (o) Record Count */
{
    long lPos;
    /* Seek to End */
    if (fseek(fpDb, 0, SEEK_END) == -1) {
        return OPENSOAP_IO_READ_ERROR;
    }
    /* get tail posotion */
    if ((lPos = ftell(fpDb)) == -1) {
        return OPENSOAP_IO_READ_ERROR;
    }
    /* Count Element */
    if (lPos % OPENSOAP_CA_DATABASE_BLKSIZE != 0) {
        /* Invalid file length */
        return OPENSOAP_IO_READ_ERROR;
    }
    *plRec = lPos / OPENSOAP_CA_DATABASE_BLKSIZE;
    /* Rewind */
    if (fseek(fpDb, 0, SEEK_SET) == -1) {
        return OPENSOAP_IO_READ_ERROR;
    }
    return OPENSOAP_NO_ERROR;
}
/***************************************************************************** 
    Function      : Decode Key File
    Return        : int
 ************************************************ Yuji Yamawaki 02.02.06 *****/
int
OPENSOAP_API
OpenSOAPSecDecodeKeyFile
(FILE*           fp,            /* (i)  File Stream */
 unsigned long*  pulLenOut,     /* (o)  Length of output data */
 unsigned char** ppucDecode)    /* (o)  Decoded String(call free() after use) */
{
    int   nRet = OPENSOAP_NO_ERROR;
    int   nLineLen;
    int   nAllLen;
    int   isEffLine;
    char  szLineBuf[256];
    char* szInData = NULL;
    char* szInNew;
    unsigned long  ulLenDec;         /* Decoded Data's Length */
    unsigned char* pucDecode = NULL; /* Decoded Data */
    /* Check Arguments */
    if (fp == NULL || pulLenOut == NULL || ppucDecode == NULL) {
        nRet = OPENSOAP_PARAMETER_BADVALUE;
        goto FuncEnd;
    }
    /* Allocate Initial Area*/
    nAllLen = 1;
    szInNew = (char*) malloc(nAllLen);
    if (szInNew == NULL) {
        nRet = OPENSOAP_MEM_BADALLOC;
        goto FuncEnd;
    }
    *szInNew = '\0';
    szInData = szInNew;
    /* Read and add Original Data */
    while (fgets(szLineBuf, sizeof(szLineBuf), fp) != NULL) {
        nLineLen = strlen(szLineBuf);
        if (nLineLen <= 0) {
            continue;
        }
        if (strcmp(szLineBuf, OPENSOAP_PUBKEY_TAIL_STRING) == 0) {
            break;
        }
        nRet = openSOAPIsBase64Char(szLineBuf[0], &isEffLine);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        if (!isEffLine) {
            continue;
        }
        /* Add To InData Area */
        nAllLen += nLineLen;
        szInNew = (char*) realloc(szInData, nAllLen);
        if (szInNew == NULL) {
            nRet = OPENSOAP_MEM_BADALLOC;
            goto FuncEnd;
        } else {
            szInData = szInNew;
        }
        strcat(szInNew, szLineBuf);
    }
    /* Decode */
    nRet = openSOAPSecDecByBase64(szInNew,
                                  &ulLenDec,
                                  &pucDecode);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }

FuncEnd:
    if (OPENSOAP_FAILED(nRet)) {
        if (pucDecode != NULL) {
            free(pucDecode);
            pucDecode = NULL;
        }
        ulLenDec = 0;
    }
    if (szInData != NULL) {
        free(szInData);
    }
    *pulLenOut  = ulLenDec;
    *ppucDecode = pucDecode;
    return nRet;
}
/***************************************************************************** 
    Function      : Open CA Database File
    Return        : int
 ************************************************ Yuji Yamawaki 02.02.05 *****/
int
openSOAPSecCAOpenDatabase
(FILE** pFpDb)                  /* (o)  CA Database Handle */
{
    FILE* fpDb = NULL;
    int   nRet = OPENSOAP_NO_ERROR;
    char* szFileName;
    /* Check Arguments */
    if (pFpDb == NULL) {
        return OPENSOAP_PARAMETER_BADVALUE;
    }
    /* Get Filename From Environment Variables */
    szFileName = getenv(OPENSOAP_CA_DATABASE_ENV);
    if (szFileName == NULL) {
        nRet = OPENSOAP_FILEOPEN_ERROR;
        goto FuncEnd;
    }
    /* Open Database File */
    fpDb = fopen(szFileName, "r+b");
    if (fpDb == NULL) {
        fpDb = fopen(szFileName, "w+b"); /* Retry change mode */
        if (fpDb == NULL) {
            nRet = OPENSOAP_FILEOPEN_ERROR;
            goto FuncEnd;
        }
    }
FuncEnd:
    if (OPENSOAP_FAILED(nRet)) {
        if (fpDb != NULL) {
            fclose(fpDb);
            fpDb = NULL;
        }
    }
    *pFpDb = fpDb;
    return nRet;
}
/***************************************************************************** 
    Function      : Update Serial No.
    Return        : int
 ************************************************ Yuji Yamawaki 02.03.13 *****/
static int updateSerial
(unsigned long*  pulSerialNo)    /* (o)  New Serial Number */
{
    int   nRet = OPENSOAP_NO_ERROR;
    char* szDbFile;
    FILE* fpSerial = NULL; /* Serial No. File */
    char* szSerialName = NULL;
    const char*   szSerialExt = "_sno";
    /* Get Database Filename From Environment Variables */
    szDbFile = getenv(OPENSOAP_CA_DATABASE_ENV);
    if (szDbFile == NULL) {
        nRet = OPENSOAP_FILEOPEN_ERROR;
        goto FuncEnd;
    }
    /* Make Serial File Name */
    szSerialName = (char*)malloc(strlen(szDbFile) + strlen(szSerialExt) + 1);
    strcpy(szSerialName, szDbFile);
    strcat(szSerialName, szSerialExt);
    /* Open Serial File Name */
    fpSerial = fopen(szSerialName, "r+t");
    if (fpSerial == NULL) {
        /*== New File ==*/
        *pulSerialNo = 0;
        fpSerial = fopen(szSerialName, "wt");
        if (fpSerial == NULL) {
            nRet = OPENSOAP_FILEOPEN_ERROR;
            goto FuncEnd;
        }
        fprintf(fpSerial, "%lu", *pulSerialNo + 1);
    } else {
        /*== File Already Exists ==*/
        /* Read Data */
        if (fscanf(fpSerial, "%lu", pulSerialNo) != 1) {
            *pulSerialNo = 0;
        }
        /* Write New Serial No */
        rewind(fpSerial);
        fprintf(fpSerial, "%lu", *pulSerialNo + 1);
    }
FuncEnd:
    if (fpSerial != NULL) {
        fclose(fpSerial);
    }
    if (szSerialName != NULL) {
        free(szSerialName);
    }
    return nRet;
}
/***************************************************************************** 
    Function      : set to CA_RECORD
    Return        : int
 ************************************************ Yuji Yamawaki 02.02.05 *****/
static int setToCARec
(unsigned long  ulSerialNo,    /* (i)  Serial Number */
 const char*    szNameOwner,   /* (i)  Owner's Name */
 const char*    szTermDate,    /* (i)  Terminate Date */
                               /*      "YYYYMMDDHHMMSS" */
 size_t         sizPubKey,     /* (i)  Size of public key */
 const unsigned char* szPubKey, /* (i)  Public Key */
 OpenSOAPCARecPtr     pRec)     /* (o) */
{
    /* Check Length */
    if (strlen(szNameOwner) >= sizeof(pRec->szOwner)) {
        return OPENSOAP_MEM_OUTOFRANGE;
    }
    if (strlen(szTermDate) != sizeof(pRec->szEnd)) {
        return OPENSOAP_MEM_OUTOFRANGE;
    }
    if (sizPubKey > sizeof(pRec->ucKey)) {
        return OPENSOAP_MEM_OUTOFRANGE;
    }
    /* Set Value */
    memset(pRec, 0, sizeof(*pRec));
    pRec->ulSerial = ulSerialNo;
    pRec->ucEff    = 1;
    strcpy(pRec->szOwner, szNameOwner);
    strncpy(pRec->szEnd, szTermDate, sizeof(pRec->szEnd));
    pRec->usKeyLen = (unsigned short)sizPubKey;
    memcpy(pRec->ucKey, szPubKey, sizPubKey);

    return OPENSOAP_NO_ERROR;
}
/***************************************************************************** 
    Function      : Compare CA_RECORD
    Return        : int ( <  0 : Left < Right,
                          == 0 : Left == Right
                          >  0 : Left > Right)
 ************************************************ Yuji Yamawaki 02.02.05 *****/
static int compareCARec
(const OpenSOAPCARecPtr pLeft,
 const OpenSOAPCARecPtr pRight,
 int                    nCompSerial) /* (i)  Compare Serial No? */
                                     /*      (0: No, Others:Yes) */
{
    int nRes;
    if ((nRes = strcmp(pLeft->szOwner, pRight->szOwner)) != 0 ||
        nCompSerial == 0) {
        return nRes;
    }
    if        (pLeft->ulSerial < pRight->ulSerial) {
        return -1;
    } else if (pLeft->ulSerial == pRight->ulSerial) {
        return 0;
    } else
        return 1;
}
/***************************************************************************** 
    Function      : Write CA_RECORD
    Return        : int
 ************************************************ Yuji Yamawaki 02.02.05 *****/
static int writeCARec
(FILE*                  fpDb, /* (i)  CA database */
 const OpenSOAPCARecPtr pRec) /* (i)  Record */
{
    int i;
    unsigned char ucVal;
    /* Serial Number */
    for (i = 0; i < sizeof(pRec->ulSerial); i++) {
        ucVal = (unsigned char)(pRec->ulSerial >> 8 * (3 - i));
        if (fwrite(&ucVal, sizeof(ucVal), 1, fpDb) != 1) {
            return OPENSOAP_IO_WRITE_ERROR;
        }
    }
    /* Reserved Data */
    ucVal = 0;
    for (i = 0; i < 3; i++) {
        if (fwrite(&ucVal, sizeof(ucVal), 1, fpDb) != 1) {
            return OPENSOAP_IO_WRITE_ERROR;
        }
    }
    /* Effective */
    if (fwrite(&pRec->ucEff, sizeof(pRec->ucEff), 1, fpDb) != 1) {
        return OPENSOAP_IO_WRITE_ERROR;
    }
    /* Owner's Name */
    if (fwrite(pRec->szOwner, sizeof(pRec->szOwner), 1, fpDb) != 1) {
        return OPENSOAP_IO_WRITE_ERROR;
    }
    /* Start Date(Dummy) */
    if (fwrite(pRec->szStart, sizeof(pRec->szStart), 1, fpDb) != 1) {
        return OPENSOAP_IO_WRITE_ERROR;
    }
    /* End Date */
    if (fwrite(pRec->szEnd, sizeof(pRec->szEnd), 1, fpDb) != 1) {
        return OPENSOAP_IO_WRITE_ERROR;
    }
    /* Reserved Data */
    ucVal = 0;
    for (i = 0; i < 2; i++) {
        if (fwrite(&ucVal, sizeof(ucVal), 1, fpDb) != 1) {
            return OPENSOAP_IO_WRITE_ERROR;
        }
    }
    /* Key Length */
    for (i = 0; i < sizeof(pRec->usKeyLen); i++) {
        ucVal = (unsigned char)(pRec->usKeyLen >> 8 * (1 - i));
        if (fwrite(&ucVal, sizeof(ucVal), 1, fpDb) != 1) {
            return OPENSOAP_IO_WRITE_ERROR;
        }
    }
    /* Public Key */
    if (fwrite(pRec->ucKey, sizeof(pRec->ucKey), 1, fpDb) != 1) {
        return OPENSOAP_IO_WRITE_ERROR;
    }
    return OPENSOAP_NO_ERROR;
}
/***************************************************************************** 
    Function      : Read CA_RECORD
    Return        : int
 ************************************************ Yuji Yamawaki 02.02.05 *****/
static int readCARec
(FILE*            fpDb, /* (i)  CA database */
 OpenSOAPCARecPtr pRec) /* (o)  Record */
{
    int i;
    unsigned char ucVal;
    /* Serial Number */
    pRec->ulSerial = 0;
    for (i = 0; i < sizeof(pRec->ulSerial); i++) {
        pRec->ulSerial <<= 8;
        if (fread(&ucVal, sizeof(ucVal), 1, fpDb) != 1) {
            return OPENSOAP_IO_READ_ERROR;
        }
        pRec->ulSerial |= ucVal;
    }
    /* Reserved(SKIP) */
    if (fseek(fpDb, 3, SEEK_CUR) == -1) {
        return OPENSOAP_IO_READ_ERROR;
    }
    /* Effective */
    if (fread(&pRec->ucEff, sizeof(pRec->ucEff), 1, fpDb) != 1) {
        return OPENSOAP_IO_READ_ERROR;
    }
    /* Owner's Name */
    if (fread(pRec->szOwner, sizeof(pRec->szOwner), 1, fpDb) != 1) {
        return OPENSOAP_IO_READ_ERROR;
    }
    /* Start Date */
    if (fread(pRec->szStart, sizeof(pRec->szStart), 1, fpDb) != 1) {
        return OPENSOAP_IO_READ_ERROR;
    }
    /* End Date */
    if (fread(pRec->szEnd, sizeof(pRec->szEnd), 1, fpDb) != 1) {
        return OPENSOAP_IO_READ_ERROR;
    }
    /* Reserved(SKIP) */
    if (fseek(fpDb, 2, SEEK_CUR) == -1) {
        return OPENSOAP_IO_READ_ERROR;
    }
    /* Key Length */
    pRec->usKeyLen = 0;
    for (i = 0; i < sizeof(pRec->usKeyLen); i++) {
        pRec->usKeyLen <<= 8;
        if (fread(&ucVal, sizeof(ucVal), 1, fpDb) != 1) {
            return OPENSOAP_IO_READ_ERROR;
        }
        pRec->usKeyLen |= ucVal;
    }
    /* Public key */
    if (fread(pRec->ucKey, sizeof(pRec->ucKey), 1, fpDb) != 1) {
        return OPENSOAP_IO_READ_ERROR;
    }
    return OPENSOAP_NO_ERROR;
}
/***************************************************************************** 
    Function      : Find Position To Insert
    Return        : int
 ************************************************ Yuji Yamawaki 02.02.05 *****/
static int findInsPos
(FILE*                  fpDb,   /* (i)  CA database */
 long                   lRec,   /* (i)  Total Record Count */
 const OpenSOAPCARecPtr pRec,   /* (i)  Data to Insert */
 long*                  plPos)  /* (o)  Position(Record No.) */
{
    long          lLow, lHigh, lMid;
    int           nRet, nCmpRes;
    OpenSOAPCARec rec;
    if (lRec == 0) {
        *plPos = 0;
        return OPENSOAP_NO_ERROR;
    }
    lLow  = 0;
    lHigh = lRec - 1;
    while (lLow <= lHigh) {
        lMid = (lLow + lHigh) / 2;
        /* Read at Mid Position */
        if (fseek(fpDb,
                  lMid * OPENSOAP_CA_DATABASE_BLKSIZE,
                  SEEK_SET) == -1) {
            return OPENSOAP_IO_READ_ERROR;
        }
        nRet = readCARec(fpDb, &rec);
        if (OPENSOAP_FAILED(nRet)) {
            return nRet;
        }
        nCmpRes = compareCARec(pRec, &rec, 1);
        if (nCmpRes == 0) {
            *plPos = lMid;
            return OPENSOAP_NO_ERROR;
        } else if (nCmpRes < 0) {
            lHigh = lMid - 1;
        } else {
            lLow = lMid + 1;
        }
    }
    *plPos = lLow;
    return OPENSOAP_NO_ERROR;
}
/***************************************************************************** 
    Function      : Shift Record
    Return        : int
 ************************************************ Yuji Yamawaki 02.02.05 *****/
static int shiftRecord
(FILE* fpDb,           /* (i)  CA database */
 long  lRec,           /* (i)  Total Record Count */
 long  lPos)           /* (i)  Shift Start Block No. */
{
    long i;
    unsigned char szDataBuf[OPENSOAP_CA_DATABASE_BLKSIZE];

    if (lRec == 0) {
        return OPENSOAP_NO_ERROR;
    }

    for (i = lRec - 1; i >= lPos; i--) {
        /* Shift to Original Position */
        if (fseek(fpDb,
                  (long)OPENSOAP_CA_DATABASE_BLKSIZE * (long)i,
                  SEEK_SET) == -1) {
            return OPENSOAP_IO_READ_ERROR;
        }
        /* Read Original Data */
        if (fread(szDataBuf, sizeof(szDataBuf), 1, fpDb) != 1) {
            return OPENSOAP_IO_READ_ERROR;
        }
        /* change mode */
        if (fseek(fpDb,
                  (long)0,
                  SEEK_CUR) == -1) {
            return OPENSOAP_IO_READ_ERROR;
        }
        /* Write Original Data to Next Block */
        if (fwrite(szDataBuf, sizeof(szDataBuf), 1, fpDb) != 1) {
            return OPENSOAP_IO_WRITE_ERROR;
        }
    }
    return OPENSOAP_NO_ERROR;
}
/***************************************************************************** 
    Function      : Regist Key To CA Databse
    Return        : int
 ************************************************ Yuji Yamawaki 02.02.05 *****/
int
OPENSOAP_API
OpenSOAPSecCARegist
(const char*    szNameOwner,    /* (i)  Owner's Name */
 const char*    szTermDate,     /* (i)  Terminate Date */
                                /*      "YYYYMMDDHHMMSS" */
 size_t         sizPubkey,      /* (i)  Size of public key */
 const unsigned char* szPubKey, /* (i)  Public Key */
 unsigned long* pulSerialNo)    /* (o)  Serial Number */
{
    FILE*         fpDb = NULL;
    int           nRet = OPENSOAP_NO_ERROR;
    long          lRec;        /* Record Count */
    OpenSOAPCARec rec;
    long          lInsPos;
    unsigned long ulSerial;

    /* Check Arguments */
    if (szNameOwner == NULL || szTermDate == NULL ||
        sizPubkey == 0 || szPubKey == NULL || pulSerialNo == NULL) {
        return OPENSOAP_PARAMETER_BADVALUE;
    }
    if (strlen(szNameOwner) >= OPENSOAP_CA_OWNER_LEN ||
        strlen(szNameOwner) == 0) {
        return OPENSOAP_PARAMETER_BADVALUE;
    }
    /* Open Database */
    nRet = openSOAPSecCAOpenDatabase(&fpDb);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Get record count */
    nRet = countRec(fpDb, &lRec);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Update Serial No */
    nRet = updateSerial(&ulSerial);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* make record */
    nRet = setToCARec(ulSerial,
                      szNameOwner,
                      szTermDate,
                      sizPubkey,
                      szPubKey,
                      &rec);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Find Insert Position */
    nRet = findInsPos(fpDb, lRec, &rec, &lInsPos);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Shift Record */
    nRet = shiftRecord(fpDb, lRec, lInsPos);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Write New Record */
    if (fseek(fpDb,
              (long)(lInsPos * OPENSOAP_CA_DATABASE_BLKSIZE),
              SEEK_SET) == -1) {
        nRet = OPENSOAP_IO_READ_ERROR;
        goto FuncEnd;
    }
    nRet = writeCARec(fpDb, &rec);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Set Serial Number */
    *pulSerialNo = (unsigned long)lRec;
FuncEnd:
    if (fpDb != NULL) {
        fclose(fpDb);
    }
    return nRet;
}
/***************************************************************************** 
    Function      : Get same name record count
    Return        : int
 ************************************************ Yuji Yamawaki 02.02.06 *****/
static int getSameNameRecordCount
(FILE*          fpDb,           /* (i)  CA database */
 unsigned long  ulRecAll,       /* (i)  All Record Count */
 const char*    szNameOwner,    /* (i)  Owner's Name */
 unsigned long  ulPosFound,     /* (i)  Found Position */
 unsigned long* pulPosTop,      /* (o)  Top Position */
 int*           pnCntFound,     /* (o)  Found Record Count */
 int*           pnCntEff)       /* (o)  Effective Record Count */
{
    unsigned long i;
    unsigned long ulPosCur;
    OpenSOAPCARec rec;
    int           nRet = OPENSOAP_NO_ERROR;
    /* Find top position */
    if (ulPosFound == 0) {
        *pulPosTop = 0;
    } else {
        ulPosCur = ulPosFound - 1;
        for (;;) {
            /* seek & read */
            if (fseek(fpDb,
                      ulPosCur * OPENSOAP_CA_DATABASE_BLKSIZE,
                      SEEK_SET) == -1) {
                return OPENSOAP_IO_READ_ERROR;
            }
            nRet = readCARec(fpDb, &rec);
            if (OPENSOAP_FAILED(nRet)) {
                return nRet;
            }
            /* Compare Owner's Name */
            if (strcmp(rec.szOwner, szNameOwner) != 0) {
                /* Differ --> Found Top */
                *pulPosTop = ulPosCur + 1;
                break;
            }
            if (ulPosCur == 0) {
                *pulPosTop = 0;
                break;
            }
            /* Update Position */
            ulPosCur--;
        }
    }
    /* Get Record Count */
    *pnCntFound = 0;
    *pnCntEff   = 0;
    if (fseek(fpDb,
              (*pulPosTop) * OPENSOAP_CA_DATABASE_BLKSIZE,
              SEEK_SET) == -1) {
        return OPENSOAP_IO_READ_ERROR;
    }
    for (i = *pulPosTop; i < ulRecAll; i++) {
        nRet = readCARec(fpDb, &rec);
        if (OPENSOAP_FAILED(nRet)) {
            return nRet;
        }
        if (strcmp(rec.szOwner, szNameOwner) != 0) {
            break;
        }
        (*pnCntFound)++;
        if (rec.ucEff)
            (*pnCntEff)++;
    }
    return nRet;
}
/***************************************************************************** 
    Function      : Find record
    Return        : int
 ************************************************ Yuji Yamawaki 02.02.06 *****/
static int findRecord
(FILE*          fpDb,           /* (i)  CA database */
 const char*    szNameOwner,    /* (i)  Owner's Name */
 unsigned long  ulSerialNo,     /* (i)  Serial No(0xffffffff: Search Minimum) */
 long*          plPos,          /* (o)  Position(Record No.) */
 int*           pnCntFound,     /* (o)  found data count */
 int*           pnCntEff)       /* (o)  effective data count */
{
    int           nRet = OPENSOAP_NO_ERROR;
    long          lRecAll;
    long          lLow, lHigh, lMid;
    int           nCmpSerial;
    int           nCmpRes;
    OpenSOAPCARec recRead, recCmp;
    /* Count Records */
    nRet = countRec(fpDb, &lRecAll);
    if (OPENSOAP_FAILED(nRet)) {
        return nRet;
    }
    if (lRecAll == 0) {
        /* Empty */
        *pnCntFound = 0;
        *pnCntEff   = 0;
        return nRet;
    }
    /* Set Search Condition */
    if (ulSerialNo == 0xffffffff) {
        nCmpSerial = 0;
    } else {
        nCmpSerial = 1;
    }
    /* Search */
    lLow = 0;
    lHigh = lRecAll - 1;
    strcpy(recCmp.szOwner, szNameOwner);
    recCmp.ulSerial = ulSerialNo;
    while (lLow <= lHigh) {
        lMid = (lLow + lHigh) / 2;
        /* Read at Mid Position */
        if (fseek(fpDb,
                  lMid * OPENSOAP_CA_DATABASE_BLKSIZE,
                  SEEK_SET) == -1) {
            return OPENSOAP_IO_READ_ERROR;
        }
        nRet = readCARec(fpDb, &recRead);
        if (OPENSOAP_FAILED(nRet)) {
            return nRet;
        }
        nCmpRes = compareCARec(&recCmp, &recRead, nCmpSerial);
        if (nCmpRes == 0) {
            /* Found!! */
            if (nCmpSerial) {
                /* With Serial --> Found 1 Element */
                *plPos = lMid;
                *pnCntFound = 1;
                if (recRead.ucEff) {
                    *pnCntEff = 1;
                } else {
                    *pnCntEff = 0;
                }
                return OPENSOAP_NO_ERROR;
            } else {
                /* No Serial --> Search Count */
                return getSameNameRecordCount(fpDb,
                                              lRecAll,
                                              szNameOwner,
                                              lMid,
                                              plPos,
                                              pnCntFound,
                                              pnCntEff);
            }
        } else if (nCmpRes < 0) {
            lHigh = lMid - 1;
        } else {
            lLow = lMid + 1;
        }
    }
    /* Not Found */
    *pnCntFound = 0;
    *pnCntEff = 0;
    return nRet;
}
/***************************************************************************** 
    Function      : Search Records
    Return        : int
 ************************************************ Yuji Yamawaki 02.02.06 *****/
int
OPENSOAP_API
OpenSOAPSecCASearchRecords
(const char* szNameOwner, /* (i)  Owner's Name */
 int*        pnRec,       /* (o)  Record Count */
 long**      pplIdxs)     /* (o)  Index Numbers (call free() after use) */
{
    FILE*         fpDb = NULL;
    int           nRet = OPENSOAP_NO_ERROR;
    int           nRecFound;
    int           nRecEff;
    unsigned long ulPosTop;
    OpenSOAPCARec rec;
    int           i;
    int           nPackPos;
    long*         plNew = NULL;

    /* Check Parameters */
    if (szNameOwner == NULL ||
        pnRec == NULL || pplIdxs == NULL) {
        return OPENSOAP_PARAMETER_BADVALUE;
    }
    /* Open Database */
    nRet = openSOAPSecCAOpenDatabase(&fpDb);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Find Record */
    nRet = findRecord(fpDb,
                      szNameOwner,
                      (unsigned long)0xffffffff,
                      &ulPosTop,
                      &nRecFound,
                      &nRecEff);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    if (nRecEff > 0) {
        /* Allocate Output Area */
        plNew = (long*) malloc(sizeof(long) * nRecEff);
        if (plNew == NULL) {
            nRet = OPENSOAP_MEM_BADALLOC;
            goto FuncEnd;
        }
        /* Get Index Numbers */
        if (fseek(fpDb,
                  ulPosTop * OPENSOAP_CA_DATABASE_BLKSIZE,
                  SEEK_SET) == -1) {
            nRet = OPENSOAP_IO_READ_ERROR;
            goto FuncEnd;
        }
        nPackPos = 0;
        for (i = 0; i < nRecFound; i++) {
            nRet = readCARec(fpDb, &rec);
            if (OPENSOAP_FAILED(nRet)) {
                goto FuncEnd;
            }
            if (rec.ucEff) {
                plNew[nPackPos++] = (long)ulPosTop + (long)i;
            }
        }
    }
FuncEnd:
    if (fpDb != NULL) {
        fclose(fpDb);
    }
    if (OPENSOAP_FAILED(nRet)) {
        if (plNew != NULL) {
            free(plNew);
            plNew = NULL;
        }
        nRecEff = 0;
    }
    *pnRec = nRecEff;
    *pplIdxs = plNew;
    return nRet;
}
/***************************************************************************** 
    Function      : Get Record (By Index)
    Return        : int
 ************************************************ Yuji Yamawaki 02.03.12 *****/
int
OPENSOAP_API
OpenSOAPSecCAGetRecord
(long              lIdx,    /* (i)  Index */
 OpenSOAPCARecPtr* ppRec)   /* (o)  Record */
{
    FILE*            fpDb = NULL;
    int              nRet = OPENSOAP_NO_ERROR;
    OpenSOAPCARecPtr pNew = NULL;
    long             lRec;
    /* Check Parameters */
    if (lIdx < 0 || ppRec == NULL) {
        return OPENSOAP_PARAMETER_BADVALUE;
    }
    /* Open Database */
    nRet = openSOAPSecCAOpenDatabase(&fpDb);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Count Record */
    nRet = countRec(fpDb, &lRec);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Verify Count */
    if (lIdx >= lRec) {
        nRet = OPENSOAP_PARAMETER_BADVALUE;
        goto FuncEnd;
    }
    /* Allocate New Area */
    pNew = (OpenSOAPCARecPtr)malloc(sizeof(OpenSOAPCARec));
    if (pNew == NULL) {
        nRet = OPENSOAP_MEM_BADALLOC;
        goto FuncEnd;
    }
    /* Read Record */
    if (fseek(fpDb,
              lIdx * OPENSOAP_CA_DATABASE_BLKSIZE,
              SEEK_SET) == -1) {
        nRet = OPENSOAP_IO_READ_ERROR;
        goto FuncEnd;
    }
    nRet = readCARec(fpDb, pNew);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }

FuncEnd:
    if (fpDb != NULL) {
        fclose(fpDb);
    }
    if (OPENSOAP_FAILED(nRet)) {
        OpenSOAPSecCAFreeRecord(pNew);
        pNew = NULL;
    }
    *ppRec = pNew;
    return nRet;
}
/***************************************************************************** 
    Function      : Search Record(Longest Period)
    Return        : int(if not found, returns OPENSOAP_NOT_CATEGORIZE_ERROR)
 ************************************************ Yuji Yamawaki 02.02.06 *****/
int
OPENSOAP_API
OpenSOAPSecCASearchOneRecord
(const char*         szNameOwner, /* (i)  Owner's Name */
 OpenSOAPCARecPtr*   ppRec)       /* (o)  Record */
{
    FILE*             fpDb = NULL;
    int               nRet = OPENSOAP_NO_ERROR;
    long*             plIdxs = NULL; /* Index Group */
    OpenSOAPCARecPtr* ppRecs = NULL; /* Found Records */
    int               nRec;
    OpenSOAPCARecPtr  pNew = NULL;
    int               i;

    /* Check Parameters */
    if (szNameOwner == NULL || ppRec == NULL) {
        return OPENSOAP_PARAMETER_BADVALUE;
    }
    /* search records */
    nRet = OpenSOAPSecCASearchRecords(szNameOwner,
                                      &nRec,
                                      &plIdxs);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    if (nRec <= 0) {
        nRet = OPENSOAP_NOT_CATEGORIZE_ERROR;
        goto FuncEnd;
    }
    /* Allocate Data Area */
    ppRecs = (OpenSOAPCARecPtr*)calloc(nRec, sizeof(OpenSOAPCARecPtr));
    if (ppRecs == NULL) {
        nRet = OPENSOAP_MEM_BADALLOC;
        goto FuncEnd;
    }
    /* Load All Data */
    for (i = 0; i < nRec; i++) {
        nRet = OpenSOAPSecCAGetRecord(plIdxs[i], ppRecs + i);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
    }
    pNew = (OpenSOAPCARecPtr)malloc(sizeof(OpenSOAPCARec));
    if (pNew == NULL) {
        nRet = OPENSOAP_MEM_BADALLOC;
        goto FuncEnd;
    }
    /* find longest period's record */
    *pNew = *(ppRecs[0]);
    for (i = 1; i < nRec; i++) {
        if (strncmp(pNew->szEnd,
                    ppRecs[i]->szEnd,
                    sizeof(pNew->szEnd)) < 0) {
            *pNew = *(ppRecs[i]);
        }
    }
FuncEnd:
    if (ppRecs != NULL) {
        for (i = 0; i < nRec; i++) {
            OpenSOAPSecCAFreeRecord(ppRecs[i]);
        }
        free(ppRecs);
    }
    if (fpDb != NULL) {
        fclose(fpDb);
    }
    if (plIdxs != NULL) {
        free(plIdxs);
    }
    if (OPENSOAP_FAILED(nRet)) {
        OpenSOAPSecCAFreeRecord(pNew);
        pNew = NULL;
    }
    *ppRec = pNew;
    return nRet;
}
/***************************************************************************** 
    Function      : Free Record
    Return        : int
 ************************************************ Yuji Yamawaki 02.03.12 *****/
int
OPENSOAP_API
OpenSOAPSecCAFreeRecord
(OpenSOAPCARecPtr pRec)       /* (i)  Record */
{
    if (pRec != NULL) {
        free(pRec);
    }
    return OPENSOAP_NO_ERROR;
}
/***************************************************************************** 
    Function      : Invalidate Record
    Return        : int(if not found, returns OPENSOAP_NOT_CATEGORIZE_ERROR)
 ************************************************ Yuji Yamawaki 02.02.06 *****/
int
OPENSOAP_API
OpenSOAPSecCAInvalidate
(const char*     szNameOwner,   /* (i)  Owner's Name */
 unsigned long   ulSerial)      /* (i)  Serial Number */
{
    FILE*         fpDb = NULL;
    int           nRet;
    unsigned long ulPos;
    int           nCntFound, nCntEff;
    OpenSOAPCARec rec;
    /* Check Parameters */
    if (szNameOwner == NULL) {
        return OPENSOAP_PARAMETER_BADVALUE;
    }
    /* Open Database */
    nRet = openSOAPSecCAOpenDatabase(&fpDb);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Search */
    nRet = findRecord(fpDb,
                      szNameOwner,
                      ulSerial,
                      &ulPos,
                      &nCntFound,
                      &nCntEff);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    if (nCntFound <= 0) {
        nRet = OPENSOAP_NOT_CATEGORIZE_ERROR;
        goto FuncEnd;
    }
    /* Invalidate */
    if (fseek(fpDb,
              ulPos * OPENSOAP_CA_DATABASE_BLKSIZE,
              SEEK_SET) == -1) {
        nRet = OPENSOAP_IO_READ_ERROR;
        goto FuncEnd;
    }
    nRet = readCARec(fpDb, &rec);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    rec.ucEff = 0;
    if (fseek(fpDb,
              ulPos * OPENSOAP_CA_DATABASE_BLKSIZE,
              SEEK_SET) == -1) {
        nRet = OPENSOAP_IO_READ_ERROR;
        goto FuncEnd;
    }
    nRet = writeCARec(fpDb, &rec);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
FuncEnd:
    if (fpDb != NULL) {
        fclose(fpDb);
    }
    return nRet;
}
/***************************************************************************** 
    Function      : Browse Record
    Return        : int
 ************************************************ Yuji Yamawaki 02.03.12 *****/
int
OPENSOAP_API
OpenSOAPSecCABrowseRec
(const OpenSOAPCARecPtr pRec,  /* (i)  Record */
 FILE*                  fpOut) /* (i)  Output Stream */
{
    int i;
    int nRet = OPENSOAP_NO_ERROR;
    fprintf(fpOut, "%010lu", pRec->ulSerial);
    if (pRec->ucEff) {
        putc(' ', fpOut);
    } else {
        putc('*', fpOut);
    }
    fprintf(fpOut, "[%s] ", pRec->szOwner);
    for (i = 0; i < (int)sizeof(pRec->szEnd); i++) {
        putc(pRec->szEnd[i], fpOut);
    }
    putc('\n', fpOut);
    return nRet;
}
/***************************************************************************** 
    Function      : Browse CA Database
    Return        : int
 ************************************************ Yuji Yamawaki 02.02.08 *****/
int
OPENSOAP_API
OpenSOAPSecCABrowse
(FILE* fpOut)                   /* (i)  Output Stream */
{
    FILE*         fpDb = NULL;
    unsigned long ulRecCnt;
    int           nRet = OPENSOAP_NO_ERROR;
    int           i;
    OpenSOAPCARec rec;
    /* Open Database */
    nRet = openSOAPSecCAOpenDatabase(&fpDb);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Get Record Count */
    nRet = countRec(fpDb, &ulRecCnt);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    fprintf(fpOut, "***Count = %lu***\n", ulRecCnt);
    for (i = 0; i < (int)ulRecCnt; i++) {
        nRet = readCARec(fpDb, &rec);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        nRet = OpenSOAPSecCABrowseRec(&rec, fpOut);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
    }
FuncEnd:
    if (fpDb != NULL) {
        fclose(fpDb);
    }
    return nRet;
}
/***************************************************************************** 
    Function      : Remove Record
    Return        : int(if not found, returns OPENSOAP_NOT_CATEGORIZE_ERROR)
 ************************************************ Yuji Yamawaki 02.03.12 *****/
int
OPENSOAP_API
OpenSOAPSecCARemoveRecord
(unsigned long ulSerial)    /* (i)  Serial Number */
{
    int nRet = OPENSOAP_NO_ERROR;
    FILE* fpDb = NULL; /* Database File */
    FILE* fpTmp = NULL; /* Temporary File */
    long  lRecAll;
    char* szFileName;
    long  i;
    OpenSOAPCARec rec;
    int           iData;
    int           iExecRemove = 0;

    /* Get Database Name */
    szFileName = getenv(OPENSOAP_CA_DATABASE_ENV);
    if (szFileName == NULL) {
        nRet = OPENSOAP_FILEOPEN_ERROR;
        goto FuncEnd;
    }
    /* Open Database To Read */
    fpDb = fopen(szFileName, "rb");
    if (fpDb == NULL) {
        nRet = OPENSOAP_FILEOPEN_ERROR;
        goto FuncEnd;
    }
    /* Open Temporary File Name */
    fpTmp = tmpfile();
    if (fpTmp == NULL) {
        nRet = OPENSOAP_FILEOPEN_ERROR;
        goto FuncEnd;
    }
    /* Count Records */
    nRet = countRec(fpDb, &lRecAll);
    if (OPENSOAP_FAILED(nRet)) {
        return nRet;
    }
    /* Copy To Tmp File */
    for (i = 0; i < lRecAll; i++) {
        /* Read Original */
        nRet = readCARec(fpDb, &rec);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        /* Write To Temporary File Except Specified Record */
        if (rec.ulSerial != ulSerial) {
            nRet = writeCARec(fpTmp, &rec);
            if (OPENSOAP_FAILED(nRet)) {
                goto FuncEnd;
            }
        } else {
            iExecRemove = 1;    /* Executed Remove */
        }
    }
    if (iExecRemove == 0) {
        /* Specified Data Not Found */
        nRet = OPENSOAP_NOT_CATEGORIZE_ERROR;
        goto FuncEnd;
    }
    /* Write To Original */
    fclose(fpDb);
    fpDb = fopen(szFileName, "wb"); /* Re-Open To Write */
    if (fpDb == NULL) {
        nRet = OPENSOAP_FILEOPEN_ERROR;
        goto FuncEnd;
    }
    rewind(fpTmp);
    while ((iData = getc(fpTmp)) != EOF) {
        putc(iData, fpDb);
    }
    
FuncEnd:
    if (fpDb != NULL) {
        fclose(fpDb);
    }
    if (fpTmp != NULL) {
        fclose(fpTmp);
    }
    return nRet;
}
