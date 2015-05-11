/*-----------------------------------------------------------------------------
 * $RCSfile: fio.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include "tstcmn.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>

static const unsigned char* s_szCharSet = "UTF-8";

/***************************************************************************** 
    Function      : Load File to Memory
    Return        : unsigned char*(call free() afer use)
 ************************************************ Yuji Yamawaki 01.07.27 *****/
unsigned char* LoadFile
(const char*    szFileName, /* (i)  File Name */
 unsigned long* pulSize)    /* (o)  File Size(Bytes) */
{
    unsigned char* szNew = NULL;
    FILE*          fp = NULL;
    struct stat    statBuf;
    /* Check Arguments */
    if (szFileName == NULL || pulSize == NULL)
        return NULL;
    /* Get File Size */
    if (stat(szFileName, &statBuf) != 0) {
        fprintf(stderr, "File Not Found(%s)\n", szFileName);
        goto FuncEnd;
    }
    *pulSize = (unsigned long)statBuf.st_size;
    if (*pulSize == 0) {
        fprintf(stderr, "File Size = %lu\n", *pulSize);
        goto FuncEnd;
    }
    /* Allocate Area */
    if ((szNew = (unsigned char*) malloc(*pulSize)) == NULL) {
        fprintf(stderr, "Memory Allocate Error(size:%lu).\n", *pulSize);
        goto FuncEnd;
    }
    /* Open and Load File */
    if ((fp = fopen(szFileName, "rb")) == NULL) {
        fprintf(stderr, "File Open Error(%s)\n", szFileName);
        free(szNew);
        szNew = NULL;
        goto FuncEnd;
    }
    if (fread(szNew, *pulSize, 1, fp) != 1) {
        fprintf(stderr, "File Read Error(%s)\n", szFileName);
        free(szNew);
        szNew = NULL;
        goto FuncEnd;
    }
    fclose(fp);
    fp = NULL;
FuncEnd:
    if (fp != NULL)
        fclose(fp);
    return szNew;
}
/***************************************************************************** 
    Function      : Save Memory To File
    Return        : int (0: OK, Others: NG)
 ************************************************ Yuji Yamawaki 01.07.27 *****/
int SaveFile
(unsigned char* szMemData,      /* (i)  Data In Memory */
 unsigned long  ulMemSize,      /* (i)  Data Size(Bytes) */
 const char*    szFileName)     /* (i)  File Name */
{
    FILE* fp = NULL;
    int   iRet = 0;
    /* Check Arguments */
    if (szMemData == NULL || ulMemSize == 0 || szFileName == NULL)
        return -1;
    /* Open File */
    if ((fp = fopen(szFileName, "wb")) == NULL) {
        fprintf(stderr, "File Open Error(%s).\n", szFileName);
        iRet = -1;
        goto FuncEnd;
    }
    /* Write */
    if (fwrite(szMemData, ulMemSize, 1, fp) != 1) {
        fprintf(stderr, "File Write Error(%s).\n", szFileName);
        iRet = -1;
        goto FuncEnd;
    }
    /* End */
    fclose(fp);
    fp = NULL;

FuncEnd:
    if (fp != NULL)
        fclose(fp);
    return iRet;
}
/***************************************************************************** 
    Function      : Save Memory To File With New Line
    Return        : int (0: OK, Others: NG)
 ************************************************ Yuji Yamawaki 01.07.27 *****/
int SaveFileWithNewLine
(unsigned char* szMemData,      /* (i)  Data In Memory */
 unsigned long  ulMemSize,      /* (i)  Data Size(Bytes) */
 int            nCharPerLine,   /* (i)  Character Count Per Line */
 const char*    szFileName)     /* (i)  File Name */
{
    FILE* fp = NULL;
    int   iRet = 0;
    unsigned long  ulRest;
    unsigned char* szCur;
    unsigned long  ulWriteSize;

    /* Check Arguments */
    if (szMemData == NULL || ulMemSize == 0 ||
        nCharPerLine <= 0 || szFileName == NULL)
        return -1;
    /* Open File */
    if ((fp = fopen(szFileName, "wb")) == NULL) {
        fprintf(stderr, "File Open Error(%s).\n", szFileName);
        iRet = -1;
        goto FuncEnd;
    }
    /* Write */
    ulRest = ulMemSize;
    szCur = szMemData;
    while (ulRest > 0) {
        if (ulRest > (unsigned long)nCharPerLine) {
            ulWriteSize = (unsigned long)nCharPerLine;
        } else {
            ulWriteSize = ulRest;
        }
        if (fwrite(szCur, ulWriteSize, 1, fp) != 1) {
            fprintf(stderr, "File Write Error(%s).\n", szFileName);
            iRet = -1;
            goto FuncEnd;
        }
        fputc('\n', fp);
        szCur += ulWriteSize;
        ulRest -= ulWriteSize;
    }
    /* End */
    fclose(fp);
    fp = NULL;

FuncEnd:
    if (fp != NULL)
        fclose(fp);
    return iRet;
}
/***************************************************************************** 
    Function      : Save SOAP Envelope
    Return        : int
 ************************************************ Yuji Yamawaki 01.11.26 *****/
int SaveEnvelope
(const OpenSOAPEnvelopePtr soap_env,  /* (i)  SOAP Envelope */
 const char*               szFName)   /* (i)  File Name */
{
    int iRet = OPENSOAP_PARAMETER_BADVALUE;
    OpenSOAPByteArrayPtr soap_env_data = NULL;
    FILE* fp = NULL;
    size_t sizData;
    const unsigned char* szDataTop;

    if (!soap_env || !szFName) {
        /* Bad Argument */
        goto FuncEnd;
    }
    iRet = OpenSOAPByteArrayCreate(&soap_env_data);
    if (!OPENSOAP_SUCCEEDED(iRet)) {
        goto FuncEnd;
    }
    /* Envelope -> ByteArray */
    iRet = OpenSOAPEnvelopeGetCharEncodingString(soap_env,
                                                 s_szCharSet,
                                                 soap_env_data);
    if (!OPENSOAP_SUCCEEDED(iRet)) {
        goto FuncEnd;
    }
    /* Get Byte Array Size */
    iRet = OpenSOAPByteArrayGetSize(soap_env_data,
                                    &sizData);
    if (!OPENSOAP_SUCCEEDED(iRet)) {
        goto FuncEnd;
    }
    /* Get Byte Array Top */
    iRet = OpenSOAPByteArrayBeginConst(soap_env_data,
                                       &szDataTop);
    if (!OPENSOAP_SUCCEEDED(iRet)) {
        goto FuncEnd;
    }
    /* Save To File */
    fp = fopen(szFName, "wb");
    if (fp == NULL) {
        fprintf(stderr, "Cannot Open File [%s].\n", szFName);
        iRet = OPENSOAP_PARAMETER_BADVALUE;
        goto FuncEnd;
    }
    if (fwrite(szDataTop, 1, sizData, fp) != sizData) {
        fprintf(stderr, "Write Error File [%s].\n", szFName);
        iRet = OPENSOAP_PARAMETER_BADVALUE;
        goto FuncEnd;
    }
FuncEnd:
    OpenSOAPByteArrayRelease(soap_env_data);
    if (fp != NULL) {
        fclose(fp);
    }
    if (OPENSOAP_FAILED(iRet)) {
        fprintf(stderr,
                "saveEnvelope() Error. "
                "OpenSOAP Error Code: 0x%04x\n",
                iRet);
        
    }
    return iRet;
}

/***************************************************************************** 
    Function      : Load SOAP Envelope
    Return        : int
 ************************************************ Yuji Yamawaki 01.11.26 *****/
int LoadEnvelope
(const char*          szFName,   /* (i)  Envelope Filename */
 OpenSOAPEnvelopePtr* pEnv)      /* (o)  Envelope */
{
    int                  iRet = OPENSOAP_PARAMETER_BADVALUE;
    unsigned char*       szData = NULL; /* Envelope Data */
    unsigned long        ulSize;
    OpenSOAPByteArrayPtr barray = NULL;

    /* Check the arguement */
    if (szFName == NULL || pEnv == NULL) {
        goto FuncEnd;
    }
    /* Load Envelope file */
    szData = LoadFile(szFName, &ulSize);
    if (szData == NULL) {
        fprintf(stderr, "Insufficient Memory.\n");
        goto FuncEnd;
    }
    /* Convert to OpenSOAPByteArrayPtr */
    iRet = OpenSOAPByteArrayCreateWithData(szData,
                                           (size_t)ulSize,
                                           &barray);
    if (!OPENSOAP_SUCCEEDED(iRet)) {
        goto FuncEnd;
    }
    if (szData != NULL) {
        /* Delete the original data, not required any more */
        free(szData);
        szData = NULL;
    }
    /* Create Envelope from ByteArray */
    iRet = OpenSOAPEnvelopeCreateCharEncoding(s_szCharSet,
                                              barray,
                                              pEnv);
    if (!OPENSOAP_SUCCEEDED(iRet)) {
        goto FuncEnd;
    }
FuncEnd:
    if (barray != NULL) {
        OpenSOAPByteArrayRelease(barray);
    }
    if (szData != NULL) {
        free(szData);
    }
    return iRet;
}
